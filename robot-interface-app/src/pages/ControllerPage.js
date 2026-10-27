import React, { useEffect, useState } from "react";
import { Joystick } from "react-joystick-component";
import Grid from "@mui/material/Grid";
import Box from "@mui/material/Box";
import Button from "@mui/material/Button";
import KeyboardVoiceIcon from "@mui/icons-material/KeyboardVoice";
import MicOffIcon from "@mui/icons-material/MicOff";
import VideocamIcon from "@mui/icons-material/Videocam";
import VideocamOffIcon from "@mui/icons-material/VideocamOff";
import CallEndIcon from "@mui/icons-material/CallEnd";
import IconButton from "@mui/material/IconButton";
import Slider from "@mui/material/Slider";
import Typography from "@mui/material/Typography";
import Paper from "@mui/material/Paper";
import { ToggleButton } from "@mui/material";
import { Build } from "@mui/icons-material";

import useSnackbar from "../hooks/Snackbar";
import useAV from "../hooks/AV";
import useRobotControls from "../hooks/RobotControls";

import { MeetingProvider, useMeeting } from "../contexts/MeetingContext";
import { maxContraints, minContraints } from "../constants/motor_const";
import { useNavigate } from "react-router-dom";
import { VIDEO_W, VIDEO_H } from "../constants/corelink_const";

export default function ControllerPage({ connection, updateConnection }) {
  const navigate = useNavigate();

  useEffect(() => {
    if (!connection.user || !connection.workspace) {
      navigate("/prelanding");
    }
  }, [connection.user, connection.workspace, navigate]);

  if (!connection.user || !connection.workspace) {
    return null;
  }
  return (
    <MeetingProvider>
      <ControllerPageContent
        connection={connection}
        updateConnection={updateConnection}
      />
    </MeetingProvider>
  );
}
function valuetext(value) {
  return `${value}`;
}

function ControllerPageContent({ connection, updateConnection }) {
  const workspace = connection.workspace;
  console.log(connection);
  const navigate = useNavigate();

  const [showMotors, setShowMotors] = useState(false);
  const { setInMeeting, videoStreamList } = useMeeting();

  // Hooks
  const { showSnackbar, SnackbarComponent } = useSnackbar();

  const {
    toggleAudio,
    toggleVideo,
    isAudioOn,
    isVideoOn,
    videoRef,
    canvasRef,
    unloadAV,
    unloadCorelink,
    renderMediaComponent,
  } = useAV(connection.user, connection.workspace, showSnackbar);

  const {
    handleStart,
    handleMove,
    handleStop,
    renderTick,
    sliderRef,
    sliderTextRef,
    sliderValues,
    displayInput,
  } = useRobotControls();

  // Variables
  const VIDEO_WPX = `${VIDEO_W}px`;
  const VIDEO_HPX = `${VIDEO_H}px`;

  // --
  useEffect(() => {
    setInMeeting(true);
    // beforeunload: Only show confirmation dialog, do NOT cleanup here
    const handleBeforeUnload = (ev) => {
      ev.preventDefault();
      ev.returnValue = "Are you sure you want to close?";
      return ev.returnValue;
    };

    // unload: Cleanup only when user confirms leaving (after clicking "Leave" or "Reload")
    const handleUnload = () => {
      unloadAV();
      unloadCorelink(true);
    };

    window.addEventListener("beforeunload", handleBeforeUnload);
    window.addEventListener("unload", handleUnload);

    return () => {
      window.removeEventListener("beforeunload", handleBeforeUnload);
      window.removeEventListener("unload", handleUnload);
    };
  }, [unloadAV, unloadCorelink]);

  const handleEndCall = async () => {
    console.log("[ControllerPage] Ending call and returning to landing...");
    await unloadAV();
    await unloadCorelink(true);
    setInMeeting(false);
    navigate("/landing");
  };

  const applyStyle = () => {
    const videoElement = document.getElementById("video");
    if (videoElement) {
      const isFlipped = videoElement.style.transform === "rotateY(180deg)";
      if (isFlipped) {
        videoElement.style.transform = "";
        videoElement.style.WebkitTransform = "";
        videoElement.style.MozTransform = "";
      } else {
        videoElement.style.transform = "rotateY(180deg)";
        videoElement.style.WebkitTransform = "rotateY(180deg)";
        videoElement.style.MozTransform = "rotateY(180deg)";
      }
    }
  };

  return (
    <Box
      height="100vh"
      backgroundColor="var(--dark)"
      px={4}
      py={12}
      position="relative"
      boxSizing="border-box"
    >
      <Box
        position="fixed"
        top={0}
        left={0}
        backgroundColor="var(--high-transparent-white)"
        color="var(--text-bright)"
        sx={{
          display: "flex",
          flexDirection: "row",
          gap: 4,
          alignItems: "center",
          py: 2,
          px: 4,
          width: "100%",
          boxSizing: "border-box",
        }}
      >
        Workspace Name{" : "}
        {workspace}
        <Box id="controls" sx={{ display: "flex", gap: 2 }}>
          {/* <div id='dock' className='button'>Dock</div>
        <div id='undock' className='button'>Restart</div> */}
          <Button
            id="mirror"
            variant="contained"
            onClick={applyStyle}
            sx={{
              backgroundColor: "var(--secondary-color)",
              "&:hover": { backgroundColor: "var(--dark-secondary)" },
            }}
          >
            Mirror Video
          </Button>
          <ToggleButton
            value="motors"
            selected={!showMotors}
            onChange={() => setShowMotors(!showMotors)}
            sx={{
              backgroundColor: "var(--tertiary-color)",
              "&:hover": {
                backgroundColor: "var(--tertiary-color)",
                filter: "brightness(0.9)",
              },
              color: "white",
              "&.Mui-selected": {
                color: "white",
                "&:hover": {
                  filter: "brightness(0.9)",
                },
              },
            }}
          >
            <Build size="small" />
          </ToggleButton>
        </Box>
      </Box>

      <div className="maindiv">
        <Grid container spacing={3}>
          <Grid item xs={4}>
            <React.Fragment key="myVideo">
              {isVideoOn ? (
                <>
                  <video
                    width={VIDEO_WPX}
                    ref={videoRef}
                    id="video"
                    autoPlay
                    muted
                  ></video>
                  <canvas
                    width={VIDEO_WPX}
                    height={VIDEO_HPX}
                    ref={canvasRef}
                    hidden
                  ></canvas>
                </>
              ) : (
                <>
                  <video
                    width={VIDEO_WPX}
                    ref={videoRef}
                    id="video"
                    autoPlay
                    muted
                    hidden
                  ></video>
                  <canvas
                    width={VIDEO_WPX}
                    height={VIDEO_HPX}
                    ref={canvasRef}
                  ></canvas>
                </>
              )}
            </React.Fragment>
          </Grid>

          {videoStreamList.length > 0 ? (
            <>
              {videoStreamList.map((obj, i) => {
                return (
                  <Grid item xs={4}>
                    <React.Fragment key={obj.streamID}>
                      {renderMediaComponent(obj.streamID, obj.user, i)}
                    </React.Fragment>
                  </Grid>
                );
              })}
            </>
          ) : (
            <></>
          )}
        </Grid>
      </div>

      <div hidden={!showMotors}>
        <Box
          sx={{
            position: "fixed",
            top: 16,
            right: 16,
            p: 2,
            zIndex: 1300, // stay on top
          }}
        >
          <Paper elevation={4} sx={{ p: 2, borderRadius: 2 }}>
            <Typography variant="h6" gutterBottom>
              Motor Controls
            </Typography>

            <Box sx={{ display: "flex", flexDirection: "row", gap: 2 }}>
              {[0, 1, 2, 3, 4].map((val) => (
                <Box
                  key={val}
                  sx={{
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                  }}
                >
                  <Typography>{val + 1}</Typography>
                  <Slider
                    ref={(el) => {
                      sliderRef.current[val] = el;
                    }}
                    key={`slider-${val}-${renderTick}`}
                    orientation="vertical"
                    valueLabelDisplay="auto"
                    defaultValue={sliderValues.current[val]}
                    aria-label={`Slider ${val + 1}`}
                    getAriaValueText={valuetext}
                    min={minContraints[val]}
                    max={maxContraints[val]}
                    step={1}
                    sx={{
                      height: 300,
                      pointerEvents: "none",
                      "& input": { pointerEvents: "auto" },
                      "& .MuiSlider-thumb": { display: "none" },
                    }}
                  />
                  <div
                    ref={(el) => {
                      sliderTextRef.current[val] = el;
                    }}
                  >
                    null
                  </div>
                </Box>
              ))}
            </Box>
          </Paper>
        </Box>
      </div>

      <Box
        position="fixed"
        bottom={0}
        left={0}
        backgroundColor="var(--high-transparent-white)"
        sx={{
          py: 2,
          px: 4,
          width: "100%",
          boxSizing: "border-box",
        }}
      >
        <Grid
          container
          direction="row"
          justifyContent="space-between"
          alignItems="center"
        >
          <Grid item>
            <Grid container spacing={7}>
              <Grid item>
                <IconButton
                  onClick={toggleAudio}
                  sx={{
                    bgcolor: "var(--dark-muted)",
                    "&:hover": { bgcolor: "var(--dark)" },
                  }}
                >
                  {isAudioOn ? (
                    <KeyboardVoiceIcon sx={{ color: "var(--text-bright)" }} />
                  ) : (
                    <MicOffIcon sx={{ color: "var(--text-bright)" }} />
                  )}
                </IconButton>
              </Grid>
              <Grid item>
                <IconButton
                  onClick={toggleVideo}
                  sx={{
                    bgcolor: "var(--dark-muted)",
                    "&:hover": { bgcolor: "var(--dark)" },
                  }}
                >
                  {isVideoOn ? (
                    <VideocamIcon sx={{ color: "var(--text-bright)" }} />
                  ) : (
                    <VideocamOffIcon sx={{ color: "var(--text-bright)" }} />
                  )}
                </IconButton>
              </Grid>
              <Grid item>
                <IconButton
                  onClick={handleEndCall}
                  sx={{
                    bgcolor: "red",
                    "&:hover": { bgcolor: "var(--alert)" },
                  }}
                >
                  <CallEndIcon sx={{ color: "white" }} />
                </IconButton>
              </Grid>
            </Grid>
          </Grid>
          <Box
            sx={{
              display: "flex",
              flexDirection: "row",
              alignItems: "center",
              gap: 2,
            }}
          >
            <Typography color="var(--text-bright)">Battery: XX%</Typography>
            <Grid item xs={4}>
              <Joystick
                start={handleStart}
                move={handleMove}
                stop={handleStop}
                pos={displayInput}
                size={75}
                stickColor="var(--accent-color)"
              />
            </Grid>
          </Box>
          {SnackbarComponent}
        </Grid>
      </Box>
    </Box>
  );
}
