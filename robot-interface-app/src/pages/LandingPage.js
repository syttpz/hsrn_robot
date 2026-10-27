import React, { useEffect, useState, useCallback } from "react";
import CreateMeetingModal from "../components/modals/CreateMeetingModal";
import JoinMeetingModal from "../components/modals/JoinMeetingModal";
import Typography from "@mui/material/Typography";
import { useNavigate } from "react-router-dom";
import { Button, Box, Card, CardContent, Stack, Chip } from "@mui/material";
import AddCircleOutlineIcon from "@mui/icons-material/AddCircleOutline";
import GroupAddIcon from "@mui/icons-material/GroupAdd";
import PeopleIcon from "@mui/icons-material/People";
import corelink from "../corelink.lib"; // added import
import { clearCorelinkCredentials } from "../utils/corelinkUtils";
import { MEETING_WORKSPACES, VIDEO_TYPE } from "../constants/corelink_const";
import { Refresh, Sync, Update } from "@mui/icons-material";

export default function LandingPage({ connection, updateConnection }) {
  const STATUS_POLL_INTERVAL_MS = 10000; // 10 seconds
  const navigate = useNavigate();
  const [activeWorkspaces, setActiveWorkspaces] = useState([]);
  const [inactiveWorkspaces, setInactiveWorkspaces] = useState([]);
  const [userCounts, setUserCounts] = useState({});
  const [loading, setLoading] = useState(true);

  const checkMeetingSessions = useCallback(async () => {
    setLoading(true);
    try {
      const active = [];
      const inactive = [];
      const counts = {};

      for (const ws of MEETING_WORKSPACES) {
        try {
          const streams = await corelink.listStreams({ workspaces: ws });
          const senders = streams?.senderList || [];
          const videoSenders = senders.filter((s) => {
            const type = Array.isArray(s.type) ? s.type[0] : s.type;
            return type === VIDEO_TYPE;
          });
          if (videoSenders.length > 0) {
            active.push(ws);
            counts[ws] = videoSenders.length;
          } else {
            inactive.push(ws);
          }
        } catch (err) {
          console.log(`Error checking workspace ${ws}:`, err);
          inactive.push(ws);
        }
      }

      setActiveWorkspaces(active);
      setInactiveWorkspaces(inactive);
      setUserCounts(counts);
    } catch (err) {
      console.error("Error checking meeting sessions:", err);
      setInactiveWorkspaces([...MEETING_WORKSPACES]);
    } finally {
      await new Promise((resolve) => setTimeout(resolve, 200));
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    if (!connection.smSenderID) {
      navigate("/prelanding");
    }
  }, [connection.smSenderID, navigate]);

  useEffect(() => {
    if (!connection.smSenderID) return;

    checkMeetingSessions();

    const intervalId = setInterval(() => {
      checkMeetingSessions();
    }, STATUS_POLL_INTERVAL_MS);

    return () => clearInterval(intervalId);
  }, [connection.smSenderID, checkMeetingSessions]);

  const handleSignOut = async () => {
    console.log("Signing out...");
    try {
      if (corelink && typeof corelink.exit === "function") {
        await corelink.exit();
        console.log("Corelink exit successful");
      }
    } catch (err) {
      console.log("Corelink exit skipped or disconnected:", err?.message);
    }

    clearCorelinkCredentials();

    updateConnection({
      smSenderID: "",
      user: "",
      socket: null,
      ws: null,
      token: null,
      workspace: "",
    });

    window.location.href = "/";

    console.log("Sign out complete");
  };

  return (
    <Box
      position="relative"
      sx={{
        p: { xs: 4, sm: 12 },
        pt: { xs: 18, sm: 16 },
      }}
    >
      {/* Header Section */}
      <Box
        position="fixed"
        top={0}
        left={0}
        display="flex"
        flexDirection="row"
        justifyContent="space-between"
        alignItems="center"
        bgcolor="var(--transparent-white)"
        sx={{
          px: { xs: 4, sm: 8 },
          py: 2,
          width: "100%",
          boxSizing: "border-box",
        }}
        boxShadow={5}
      >
        <Box display="flex" alignItems="center">
          <img
            src="/nyu2.png"
            alt="NYU Logo"
            style={{ height: 30, marginRight: 18 }}
          />
        </Box>
        <Box display="flex" alignItems="center" gap={4}>
          <Typography color="var(--accent-color)">
            {connection.user.toUpperCase()}
          </Typography>
          <Button variant="outlined" color="error" onClick={handleSignOut}>
            Sign Out
          </Button>
        </Box>
      </Box>

      {/* Active meetings summary */}
      <Box position="relative" alignItems="flex-start" mb={4}>
        <Box position="absolute" left="-32px" mr={2}>
          {loading ? (
            <Update size={20} color="action" />
          ) : (
            <Refresh
              size={20}
              onClick={checkMeetingSessions}
              sx={{ "&:hover": { cursor: "pointer" } }}
            />
          )}
        </Box>
        {
          <>
            {activeWorkspaces && activeWorkspaces.length > 0 ? (
              <Box>
                <Typography variant="body2" color="text.secondary">
                  Active meetings:
                </Typography>
                <Stack direction="row" spacing={1} flexWrap="wrap">
                  {activeWorkspaces.map((ws) => (
                    <Chip
                      key={ws}
                      icon={<PeopleIcon />}
                      label={`${ws} (${userCounts[ws] || 0} user${userCounts[ws] !== 1 ? "s" : ""})`}
                      color="success"
                      variant="outlined"
                    />
                  ))}
                </Stack>
              </Box>
            ) : (
              <Typography variant="subtitle1" sx={{ fontWeight: 600 }}>
                There are no active meetings at this time. Create a meeting
                below.
              </Typography>
            )}
          </>
        }
      </Box>
      <Stack direction={{ xs: "column", md: "row" }} spacing={3}>
        {/* Create Meeting Room */}
        <Card sx={{ flex: 1, p: 2, boxShadow: 3, borderRadius: 3 }}>
          <CardContent>
            <Box display="flex" alignItems="top" mb={2}>
              <AddCircleOutlineIcon
                color="primary"
                sx={{ mr: 1, mt: "2px", size: 24 }}
              />
              <Typography variant="h6" sx={{ fontWeight: 700 }}>
                Create Meeting Room
              </Typography>
            </Box>
            <CreateMeetingModal
              label="Create Meeting Room"
              title="Create Meeting Room"
              description="Create a meeting by choosing a workspace corresponding to a data center robot. "
              connection={connection}
              updateConnection={updateConnection}
              availableWorkspaces={inactiveWorkspaces}
              loading={loading}
              checkMeetingSessions={checkMeetingSessions}
            />
          </CardContent>
        </Card>

        {/* Join Meeting Room */}
        <Card sx={{ flex: 1, p: 2, boxShadow: 3, borderRadius: 3 }}>
          <CardContent>
            <Box display="flex" alignItems="center" mb={2}>
              <GroupAddIcon color="primary" sx={{ mr: 1 }} />
              <Typography variant="h6" sx={{ fontWeight: 700 }}>
                Join Meeting Room
              </Typography>
            </Box>
            <JoinMeetingModal
              label="Join Meeting Room"
              title="Join Meeting Room"
              description="Join a meeting by workspace. Alternatively, join the meeting through the meeting link generated when the workspace was created."
              connection={connection}
              updateConnection={updateConnection}
              activeWorkspaces={activeWorkspaces}
              userCounts={userCounts}
              loading={loading}
              checkMeetingSessions={checkMeetingSessions}
            />
          </CardContent>
        </Card>
      </Stack>
    </Box>
  );
}
