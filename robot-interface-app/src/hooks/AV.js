import { useState, useEffect, useRef, useCallback } from "react";
import Grid from "@mui/material/Grid";
import { Mic, MicOff } from "@mui/icons-material";
import useCorelink from "../hooks/Corelink";
import useCanvas from "../hooks/Canvas";
import { useMeeting } from "../contexts/MeetingContext";
import { VIDEO_W, VIDEO_H } from "../constants/corelink_const";

const useAV = (user, workspace, showSnackbar) => {
  const {
    videoSenderID,
    audioSenderID,
    videoStreamList,
    setVideoStreamList,
    audioStreamList,
    setAudioStreamList,
    initAV,
    setInitAV,
  } = useMeeting();

  // States
  const [isAudioOn, setIsAudioOn] = useState(false);
  const [isVideoOn, setIsVideoOn] = useState(false);
  const [statusList, setStatusList] = useState([]);

  // Refs
  const videoRef = useRef(null);
  const canvasRef = useRef(null);
  const canvasCtxRef = useRef(null);
  const captureInterval = useRef(null);
  const audioCtx = useRef(null);
  const audioNode = useRef(null);
  const audioStreamRef = useRef(null);
  const canvasListRef = useRef([]);
  const contextListRef = useRef([]);

  const isStreaming = useRef(false);

  // Hooks
  const { initContext, setNameCanvasA, setNameCanvasB } = useCanvas();

  const { unloadCorelink, sendVideo, sendAudio, notifyVideo, notifyAudio } =
    useCorelink({
      user,
      workspace,
      audioNode,
      canvasListRef,
      contextListRef,
      isStreaming,
      setStatusList,
    });

  // Variables
  const VIDEO_WPX = `${VIDEO_W}px`;
  const VIDEO_HPX = `${VIDEO_H}px`;

  // --
  const initVideo = useCallback(async () => {
    canvasCtxRef.current = initContext(canvasRef.current);
    setNameCanvasA(user, canvasCtxRef.current);
    if (videoSenderID) {
      notifyVideo(false);
    }
  }, [videoSenderID, sendVideo]);

  const initAudio = useCallback(async () => {
    if (audioSenderID) {
      audioCtx.current = new AudioContext({ sampleRate: 48000 });
      await audioCtx.current.audioWorklet.addModule("/AudioProcessor.js");
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      audioStreamRef.current = stream;
      const micSource = audioCtx.current.createMediaStreamSource(stream);
      audioNode.current = new AudioWorkletNode(
        audioCtx.current,
        "audio-processor",
      );
      micSource.connect(audioNode.current);
      audioNode.current.connect(audioCtx.current.destination);

      audioNode.current.port.onmessage = async (event) => {
        if (isStreaming.current) {
          sendAudio(event.data.buffer);
        }
      };
      notifyAudio(false);
    }
  }, [audioSenderID, sendAudio, isStreaming]);

  const unloadAV = useCallback(async () => {
    clearInterval(captureInterval.current);
    captureInterval.current = null;

    videoRef.current?.srcObject?.getTracks().forEach((track) => track.stop());
    if (videoRef.current) videoRef.current.srcObject = null;

    audioStreamRef.current?.getTracks().forEach((track) => track.stop());
    audioStreamRef.current = null;

    audioNode.current?.disconnect();
    audioNode.current = null;

    await audioCtx.current?.close();
    audioCtx.current = null;
  }, []);

  useEffect(() => {
    if (!initAV && videoSenderID && audioSenderID) {
      initVideo();
      initAudio();
      setInitAV(true);
    }
  }, [initAV, setInitAV, initAudio, initVideo, videoSenderID, audioSenderID]);

  // Toggles

  const toggleAudio = useCallback(async () => {
    if (!audioSenderID) return;

    console.log("toggleAudio", audioSenderID);
    try {
      if (!isAudioOn) {
        notifyAudio(true);
        setIsAudioOn(true);
        isStreaming.current = true;
        showSnackbar("Audio started", isStreaming.current);
      } else {
        notifyAudio(false);
        setIsAudioOn(false);
        isStreaming.current = false;
        showSnackbar("Audio stopped");
      }
    } catch (err) {
      notifyAudio(false);
      console.log(err);
    }
  }, [
    audioSenderID,
    isAudioOn,
    setIsAudioOn,
    isStreaming,
    notifyAudio,
    showSnackbar,
  ]);

  const toggleVideo = useCallback(async () => {
    if (!videoSenderID || !videoRef.current) return;

    console.log("toggleVideo", videoSenderID);
    try {
      if (!isVideoOn) {
        try {
          setIsVideoOn(true);
          const stream = await navigator.mediaDevices.getUserMedia({
            video: true,
          });

          if (videoRef.current) {
            // Check again after async call
            videoRef.current.srcObject = stream;

            const onLoad = () => {
              videoRef.current.play();
              showSnackbar("Video started");
              if (captureInterval.current === null) {
                captureFrame();
                captureInterval.current = setInterval(() => {
                  captureFrame();
                }, 60);
                videoRef.current.removeEventListener("loadeddata", onLoad);
              }
            };
            videoRef.current.addEventListener("loadeddata", onLoad);
          }
        } catch (err) {
          setIsVideoOn(false);
          console.error("Error accessing video stream:", err);
          videoRef.current.srcObject
            ?.getTracks()
            .forEach((track) => track.stop());
          showSnackbar("Failed to start video");
        }
      } else {
        // Clean up first before changing state
        if (captureInterval.current !== null) {
          clearInterval(captureInterval.current);
          captureInterval.current = null;
        }
        notifyVideo(false);
        setNameCanvasA(user, canvasCtxRef.current);

        videoRef.current.srcObject
          ?.getTracks()
          .forEach((track) => track.stop());
        videoRef.current.pause();
        videoRef.current.srcObject = null;
        setIsVideoOn(false); // Set state after cleanup
        showSnackbar("Video stopped");
      }
    } catch (err) {
      console.log(err);
    }
  }, [
    videoSenderID,
    isVideoOn,
    setIsVideoOn,
    captureInterval,
    notifyVideo,
    showSnackbar,
  ]);

  const captureFrame = useCallback(() => {
    if (videoRef.current && canvasRef.current) {
      // videoSenderID is not null
      canvasCtxRef.current.drawImage(videoRef.current, 0, 0, VIDEO_W, VIDEO_H);

      const webpDataUrl = canvasRef.current.toDataURL("image/webp", 0.5);
      sendVideo(base64ToUint8Array(webpDataUrl));
    }
  }, [isVideoOn, sendVideo, videoRef, canvasRef]);

  function base64ToUint8Array(base64) {
    const binaryString = atob(base64.split(",")[1]);
    const length = binaryString.length;
    const arrayBuffer = new ArrayBuffer(length);
    const uint8Array = new Uint8Array(arrayBuffer);
    for (let i = 0; i < length; i++) {
      uint8Array[i] = binaryString.charCodeAt(i);
    }
    return uint8Array;
  }

  const renderMediaComponent = (streamID, user, i) => {
    if (videoStreamList) {
      return (
        <Grid item xs={4}>
          <div>
            <canvas
              width={VIDEO_WPX}
              height={VIDEO_HPX}
              ref={(el) => {
                canvasListRef.current[i] = el;
                if (el && contextListRef.current[i] === undefined) {
                  contextListRef.current[i] = initContext(el);
                  setNameCanvasB(user, contextListRef.current[i]);
                }
              }}
              autoPlay
              muted
            ></canvas>
            <br></br>
            <div
              style={{
                color: "var(--text-bright)",
                display: "flex",
                alignItems: "center",
                gap: "4px",
                paddingTop: "2px",
              }}
            >
              {statusList[i] == "Unmuted" ? (
                <Mic color="success" fontSize="small" />
              ) : (
                <MicOff color="error" fontSize="small" />
              )}
              {`${user}`}
            </div>
          </div>
        </Grid>
      );
    } else {
      return <></>;
    }
  };

  return {
    toggleAudio,
    toggleVideo,
    isAudioOn,
    isVideoOn,
    videoRef,
    canvasRef,
    unloadAV,
    unloadCorelink,
    renderMediaComponent,
  };
};

export default useAV;
