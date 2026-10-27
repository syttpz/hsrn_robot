import { useEffect, useRef } from "react";
import corelink from "../corelink.lib";

import { useMeeting } from "../contexts/MeetingContext";
import { reconnectCorelink } from "../utils/corelinkUtils";
import useCanvas from "../hooks/Canvas";
import useUnloadCorelink from "../hooks/Unload";
import {
  AUDIO_TYPE,
  VIDEO_TYPE,
  ROBOT_CONTROL_TYPE,
  VIDEO_W,
  VIDEO_H,
  ROBOT_USER_1,
  ROBOT_USER_2,
} from "../constants/corelink_const";

const useCorelink = ({
  user,
  workspace,
  audioNode,
  canvasListRef,
  contextListRef,
  isStreaming,
  setStatusList,
}) => {
  corelink.setDebug(true);

  const {
    inMeeting,
    videoSenderID,
    setVideoSenderID,
    audioSenderID,
    setAudioSenderID,
    controlSenderID,
    setControlSenderID,
    videoStreamList,
    setVideoStreamList,
    audioStreamList,
    setAudioStreamList,
  } = useMeeting();

  const { setNameCanvasB } = useCanvas();

  const videoStreamListRef = useRef(videoStreamList);
  const audioStreamListRef = useRef(audioStreamList);
  const startedInit = useRef(false);

  useEffect(() => {
    videoStreamListRef.current = videoStreamList;
    audioStreamListRef.current = audioStreamList;
  }, [videoStreamList, audioStreamList]);

  const myReceiverID = useRef(null);

  const audio_sender_type = `${AUDIO_TYPE}`;
  const video_sender_type = `${VIDEO_TYPE}`;
  const robot_control_type = `${ROBOT_CONTROL_TYPE}`;

  useEffect(() => {
    const initCorelinkStreams = async () => {
      // Try to reconnect if needed
      await reconnectCorelink();
      await unloadCorelink(true);

      // Create senders
      try {
        const videoSenderID = await corelink.createSender({
          workspace: workspace,
          protocol: "ws",
          type: video_sender_type,
        });
        setVideoSenderID(videoSenderID);

        const controlSenderID = await corelink.createSender({
          workspace: workspace,
          protocol: "ws",
          type: robot_control_type,
        });
        setControlSenderID(controlSenderID);

        const audioSenderID = await corelink.createSender({
          workspace: workspace,
          protocol: "ws",
          type: audio_sender_type,
        });
        setAudioSenderID(audioSenderID);
      } catch (e) {
        console.log(e);
      }

      // Create receiver
      try {
        console.log("Creating Receiver...");
        const response = await corelink.createReceiver({
          workspace: workspace,
          protocol: "ws",
          echo: true,
          alert: true,
          subscribe: false,
        });

        if (!response || !response.streamID || !response.streamList) {
          console.error("createReceiver returned invalid response:", response);
          return;
        }

        myReceiverID.current = response.streamID;
        console.log("createReceiver:", response.streamList);
        for (const stream of response.streamList) {
          console.log("[INIT] Processing stream:", {
            streamID: stream.streamID,
            streamType: stream.type,
            streamTypeType: typeof stream.type,
            streamUser: stream.user,
            frontendUser: user,
            expectedVideoType: video_sender_type,
            expectedAudioType: audio_sender_type,
            expectedRobotControlType: robot_control_type,
          });

          // Handle format variations in stream type
          const streamTypeMatches = (receivedType, expectedType) => {
            if (!receivedType) return false;
            const receivedStr = String(receivedType);
            const expectedStr = String(expectedType);

            if (receivedStr === expectedStr) return true;
            if (receivedStr.includes(expectedStr)) return true;

            const cleanedReceived = receivedStr
              .replace(/[\[\]'\n]/g, "")
              .trim();
            const cleanedExpected = expectedStr
              .replace(/[\[\]'\n]/g, "")
              .trim();
            if (cleanedReceived === cleanedExpected) return true;

            return false;
          };

          const isVideoType =
            streamTypeMatches(stream.type, video_sender_type) ||
            (typeof stream.type === "string" &&
              stream.type.includes("browser-video"));
          const isAudioType = streamTypeMatches(stream.type, audio_sender_type);
          const isRobotControlType = streamTypeMatches(
            stream.type,
            robot_control_type,
          );

          console.log("[INIT] Stream type matching results:", {
            streamID: stream.streamID,
            isVideoType: isVideoType,
            isAudioType: isAudioType,
            isRobotControlType: isRobotControlType,
            userMatch: stream.user !== user,
          });

          // Allow robot streams (Testuser) OR different users
          const isRobotStream =
            stream.user != user &&
            (stream.user === ROBOT_USER_1 || stream.user === ROBOT_USER_2);
          const shouldProcess =
            (isVideoType || isAudioType || isRobotControlType) &&
            (stream.user !== user || isRobotStream);

          console.log("[INIT] Should process stream:", {
            streamID: stream.streamID,
            shouldProcess: shouldProcess,
            isRobotStream: isRobotStream,
            reason: shouldProcess
              ? "Matched type and user filter"
              : isVideoType || isAudioType || isRobotControlType
                ? "Type matched but user filter blocked"
                : "Type did not match",
          });

          if (shouldProcess && stream.streamID) {
            try {
              if (isVideoType) {
                console.log(
                  "📹 [INIT] Subscribing to initial video stream:",
                  stream.streamID,
                  "type:",
                  stream.type,
                );
                await corelink.subscribe({ streamIDs: [stream.streamID] });
                const newStream = {
                  streamID: stream.streamID,
                  type: stream.type,
                  meta: stream.meta || "",
                  user: stream.user || "",
                  apps: stream.apps || [],
                };
                setVideoStreamList((prev) => {
                  console.log("Current videoStreamList:", prev);
                  console.log("Adding new stream:", newStream);
                  return [...prev, newStream];
                });
              } else if (isAudioType) {
                console.log(
                  "🔊 [INIT] Subscribing to initial audio stream:",
                  stream.streamID,
                  "type:",
                  stream.type,
                );
                await corelink.subscribe({ streamIDs: [stream.streamID] });
                const newStream = {
                  streamID: stream.streamID,
                  type: stream.type,
                  meta: stream.meta || "",
                  user: stream.user || "",
                  apps: stream.apps || [],
                };
                setAudioStreamList((prev) => {
                  console.log("Added initial audio stream:", newStream);
                  return [...prev, newStream];
                });
                setStatusList((prev) => {
                  const newList = [...prev];
                  newList[audioStreamList.length] = "Muted";
                  return newList;
                });
              } else if (isRobotControlType) {
                console.log(
                  "[INIT] Subscribing to initial robot-control stream:",
                  stream.streamID,
                  "type:",
                  stream.type,
                  "from user:",
                  stream.user,
                );
                await corelink.subscribe({ streamIDs: [stream.streamID] });
                console.log(
                  "Subscribed to robot-control stream:",
                  stream.streamID,
                  "from user:",
                  stream.user,
                );
              }
            } catch (e) {
              console.error("Error subscribing to stream:", e);
            }
          }
        }
        // setReceiverIDs(response.streamList);
      } catch (e) {
        console.log(e);
      }
    };

    const initCorelinkData = async () => {
      initCorelinkStreams().then(() => {
        corelink.on("receiver", async (data) => {
          console.log("===RECEIVER EVENT===", data);

          console.log("🔍 [RECEIVER] Processing receiver event:", {
            streamID: data.streamID,
            streamType: data.type,
            streamTypeType: typeof data.type,
            streamUser: data.user,
            frontendUser: user,
            expectedVideoType: video_sender_type,
            expectedAudioType: audio_sender_type,
            expectedRobotControlType: robot_control_type,
          });

          const streamTypeMatches = (receivedType, expectedType) => {
            if (!receivedType) return false;
            const receivedStr = String(receivedType);
            const expectedStr = String(expectedType);

            if (receivedStr === expectedStr) return true;

            if (receivedStr.includes(expectedStr)) return true;

            const cleanedReceived = receivedStr
              .replace(/[\[\]'\n]/g, "")
              .trim();
            const cleanedExpected = expectedStr
              .replace(/[\[\]'\n]/g, "")
              .trim();
            if (cleanedReceived === cleanedExpected) return true;

            return false;
          };

          const isRobotControl = streamTypeMatches(
            data.type,
            robot_control_type,
          );
          const isVideo =
            streamTypeMatches(data.type, video_sender_type) ||
            (typeof data.type === "string" &&
              data.type.includes("browser-video"));
          const isAudio = streamTypeMatches(data.type, audio_sender_type);

          // Diagnostic logging
          console.log("[RECEIVER] Stream type matching results:", {
            streamID: data.streamID,
            receivedType: data.type,
            isVideo: isVideo,
            isAudio: isAudio,
            isRobotControl: isRobotControl,
            streamUser: data.user,
            frontendUser: user,
          });

          if (data.streamID && (isVideo || isAudio || isRobotControl)) {
            let existingStream = videoStreamListRef.current.find(
              (stream) => stream.streamID === data.streamID,
            );
            if (!existingStream) {
              existingStream = audioStreamListRef.current.find(
                (stream) => stream.streamID === data.streamID,
              );
            }
            if (
              !existingStream &&
              data.streamID !== videoSenderID &&
              data.streamID !== audioSenderID &&
              data.streamID !== controlSenderID
            ) {
              try {
                if (isVideo) {
                  console.log(
                    "📹 [RECEIVER] Subscribing to video stream:",
                    data.streamID,
                    "type:",
                    data.type,
                  );
                  await corelink.subscribe({ streamIDs: [data.streamID] });
                  console.log(
                    "[RECEIVER] Successfully subscribed to video stream:",
                    data.streamID,
                  );
                  const newStream = {
                    streamID: data.streamID,
                    type: data.type,
                    meta: data.meta || "",
                    user: data.user || "",
                    apps: data.apps || [],
                  };
                  setVideoStreamList((prev) => {
                    console.log(
                      "[RECEIVER] Adding new video stream:",
                      newStream,
                    );
                    return [...prev, newStream];
                  });
                } else if (isAudio) {
                  console.log(
                    "[RECEIVER] Subscribing to audio stream:",
                    data.streamID,
                    "type:",
                    data.type,
                  );
                  await corelink.subscribe({ streamIDs: [data.streamID] });
                  const newStream = {
                    streamID: data.streamID,
                    type: data.type,
                    meta: data.meta || "",
                    user: data.user || "",
                    apps: data.apps || [],
                  };
                  setAudioStreamList((prev) => {
                    console.log(
                      " [RECEIVER] Adding new audio stream:",
                      newStream,
                    );
                    return [...prev, newStream];
                  });
                } else if (isRobotControl) {
                  console.log(
                    " [RECEIVER] Subscribing to robot-control stream:",
                    data.streamID,
                    "type:",
                    data.type,
                  );
                  await corelink.subscribe({ streamIDs: [data.streamID] });
                  console.log(
                    " [RECEIVER] Subscribed to NEW robot-control stream:",
                    data.streamID,
                    "from user:",
                    data.user,
                  );
                }
              } catch (e) {
                console.error(" [RECEIVER] Error subscribing to stream:", e);
                console.error("   StreamID:", data.streamID);
                console.error("   Stream type:", data.type);
                console.error("   Stream user:", data.user);
              }
            } else {
              console.log(
                `Stream ${data.streamID} already exists or is own sender`,
              );
            }
          }
        });

        corelink.on("sender", async (data) => {
          console.log("===SENDER EVENT===", data);

          // Comprehensive logging for stream type handling
          console.log("🔍 [SENDER] Processing sender event:", {
            senderID: data.senderID,
            streamType: data.type,
            streamTypeType: typeof data.type,
            streamUser: data.user,
            frontendUser: user,
            expectedRobotControlType: robot_control_type,
          });

          // Handle format variations in stream type (string, array string, etc.)
          const streamTypeMatches = (receivedType, expectedType) => {
            if (!receivedType) return false;
            const receivedStr = String(receivedType);
            const expectedStr = String(expectedType);

            // Exact match
            if (receivedStr === expectedStr) return true;

            // Check if expected type is contained in received type (handles "['robot-control']\n" format)
            if (receivedStr.includes(expectedStr)) return true;

            // Check if received type without brackets/whitespace matches
            const cleanedReceived = receivedStr
              .replace(/[\[\]'\n]/g, "")
              .trim();
            const cleanedExpected = expectedStr
              .replace(/[\[\]'\n]/g, "")
              .trim();
            if (cleanedReceived === cleanedExpected) return true;

            return false;
          };

          // Subscribe to robot-control senders from other users
          // Handle both "robot-control" and "['robot-control']\n" formats
          const isRobotControl = streamTypeMatches(
            data.type,
            robot_control_type,
          );

          console.log("[SENDER] Stream type matching results:", {
            senderID: data.senderID,
            receivedType: data.type,
            isRobotControl: isRobotControl,
            userMatch: data.user !== user,
            willSubscribe:
              isRobotControl && data.user !== user && data.senderID,
          });

          if (isRobotControl && data.user !== user) {
            console.log("Attempting to subscribe to senderID:", data.senderID);
            try {
              await corelink.subscribe({ streamIDs: [data.senderID] });
              console.log(
                "Successfully subscribed to robot-control sender:",
                data.senderID,
                "from user:",
                data.user,
              );
            } catch (e) {
              console.error("Error subscribing to robot-control sender:", e);
            }
          } else {
            console.log(
              "Skipping subscription - isRobotControl:",
              isRobotControl,
              ", different user:",
              data.user !== user,
            );
          }

          notifyAudio(isStreaming.current);
        });

        corelink.on("data", (streamID, data, header) => {
          // Log ALL data received for debugging
          console.log("=== DATA EVENT FIRED ===");
          console.log("StreamID:", streamID);
          console.log("Header:", header);
          console.log("Data size:", data.byteLength);
          console.log(
            "Header keys:",
            header ? Object.keys(header) : "no header",
          );

          if (header.type === "video") {
            const index = videoStreamListRef.current.findIndex(
              (x) => x.streamID === streamID,
            );
            if (index === -1) {
              console.log("[DATA] Video stream not found in list:", streamID);
              return;
            }
            const img = new Image();
            img.src = arrayBufferToBase64(data);
            img.onload = () => {
              const ctx = contextListRef.current[index];
              if (ctx) {
                ctx.drawImage(img, 0, 0, VIDEO_W, VIDEO_H);
              } else {
                console.log(
                  "[DATA] Canvas context not available at index:",
                  index,
                );
              }
            };
          }
          if (header.type === "audio") {
            const floatArray = new Float32Array(data);
            if (audioNode.current && audioNode.current.port) {
              audioNode.current.port.postMessage(floatArray, [
                floatArray.buffer,
              ]);
            }
          }
          if (header.type === "video update" && data.byteLength === 0) {
            const index = videoStreamListRef.current.findIndex(
              (x) => x.streamID === streamID,
            );
            if (index === -1 || !videoStreamListRef.current[index]) {
              console.log(
                "[DATA] Video stream not found for update:",
                streamID,
              );
              return;
            }
            const user = videoStreamListRef.current[index].user;
            const ctx = contextListRef.current[index];
            if (user && ctx) {
              setNameCanvasB(user, ctx);
            } else {
              console.log(
                "[DATA] User or context not available for video update",
              );
            }
          }
          if (header.type === "audio update") {
            const index = audioStreamListRef.current.findIndex(
              (x) => x.streamID === streamID,
            );
            if (index === -1) {
              console.log(
                "[DATA] Audio stream not found for update:",
                streamID,
              );
              return;
            }
            setStatusList((prev) => {
              const newList = [...prev];
              newList[index] = data.byteLength === 0 ? "Muted" : "Unmuted";
              return newList;
            });
          }

          // Try to decode all data to see what we're getting
          console.log("Attempting to decode data...");
          try {
            const textDecoder = new TextDecoder("utf-8");
            const jsonStr = textDecoder.decode(data);
            console.log("Decoded string:", jsonStr);
            const jsonData = JSON.parse(jsonStr);
            console.log("Parsed JSON:", jsonData);

            if (jsonData.type === "heartbeat") {
              const timestamp = new Date(
                jsonData.timestamp,
              ).toLocaleTimeString();
              console.log(
                `HEARTBEAT #${jsonData.counter} received at ${timestamp}`,
              );
            }
          } catch (e) {
            console.log("Failed to decode/parse data:", e.message);
          }
          console.log("=== END DATA EVENT ===");
        });

        corelink.on("stale", (msg) => {
          console.log("stale: ", msg);
          let type = VIDEO_TYPE;
          let index = videoStreamListRef.current.findIndex(
            (x) => x.streamID === msg.streamID,
          );
          if (index === -1) {
            index = audioStreamListRef.current.findIndex(
              (x) => x.streamID === msg.streamID,
            );
            type = AUDIO_TYPE;
          }
          if (index === -1) {
            type = null;
          }

          if (type === VIDEO_TYPE) {
            if (canvasListRef.current[index]) {
              canvasListRef.current[index].width = 0;
              canvasListRef.current.splice(index, 1);
              contextListRef.current.splice(index, 1);
            } else {
              console.warn(`Canvas at index ${index} is undefined.`);
            }
            setVideoStreamList((prev) =>
              prev.filter((stream) => stream.streamID !== msg.streamID),
            );
            console.log("Video stream removed: ", msg.streamID);
          } else if (type === AUDIO_TYPE) {
            setAudioStreamList((prev) =>
              prev.filter((stream) => stream.streamID !== msg.streamID),
            );
            console.log("Audio stream removed: ", msg.streamID);
          }
        });
      });
    };

    if (
      inMeeting &&
      !startedInit.current &&
      videoStreamListRef.current &&
      videoStreamListRef.current.length === 0 &&
      audioStreamListRef.current &&
      audioStreamListRef.current.length === 0
    ) {
      console.log("Starting initCorelinkData...");
      startedInit.current = true;
      initCorelinkData();
    }
  }, [
    inMeeting,
    setVideoStreamList,
    setAudioStreamList,
    videoStreamListRef,
    audioStreamListRef,
    myReceiverID,
    startedInit,
    audioNode,
    setNameCanvasB,
    setStatusList,
  ]);

  function arrayBufferToBase64(buffer) {
    const bytes = new Uint8Array(buffer);
    const binaryString = String.fromCharCode.apply(null, bytes);
    return "data:image/webp;base64," + btoa(binaryString);
  }

  const unloadCorelink = useUnloadCorelink(workspace);

  const sendVideo = (buffer) => {
    // console.log("Sending video buffer: ", buffer);
    corelink.send(videoSenderID, buffer, { type: "video" });
  };

  const sendAudio = (buffer) => {
    // console.log("Sending audio buffer: ", buffer);
    corelink.send(audioSenderID, buffer, { type: "audio" });
  };

  const notifyVideo = (isOn) => {
    const data = new Uint8Array(isOn ? 1 : 0);
    corelink.send(videoSenderID, data, { type: "video update" });
  };

  const notifyAudio = (isOn) => {
    const data = new Uint8Array(isOn ? 1 : 0);
    corelink.send(audioSenderID, data, { type: "audio update" });
  };

  return {
    unloadCorelink,
    sendVideo,
    sendAudio,
    notifyVideo,
    notifyAudio,
  };
};

export default useCorelink;
