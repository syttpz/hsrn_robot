import * as React from "react";
import { Box, Button, Select, MenuItem, FormControl } from "@mui/material";
import Typography from "@mui/material/Typography";
import Paper from "@mui/material/Paper";
import Modal from "@mui/material/Modal";
import TextField from "@mui/material/TextField";
import corelink from "../../corelink.lib";
import { useNavigate } from "react-router-dom";

import { MEETING_WORKSPACES } from "../../constants/corelink_const";
// import { cleanupMeetingBeforeRejoin } from '../../utils/meetingCleanup';

const style = {
  position: "absolute",
  top: "50%",
  left: "50%",
  transform: "translate(-50%, -50%)",
  width: 400,
  bgcolor: "var(--light)",
  boxShadow: 24,
  p: 4,
  borderRadius: 3,
};

const initialValWorkspace = MEETING_WORKSPACES[0]; // Temporary default value

export default function JoinMeetingModal({
  label,
  title,
  description,
  connection,
  updateConnection,
  activeWorkspaces = [],
  userCounts = {},
  loading = false,
  checkMeetingSessions = () => {},
}) {
  const [open, setOpen] = React.useState(false);
  const handleOpen = () => {
    checkMeetingSessions();
    setOpen(true);
  };
  const handleClose = () => setOpen(false);
  const navigate = useNavigate();
  const workspaces = activeWorkspaces.length > 0 ? activeWorkspaces : [];
  const [workspace, setWorkspace] = React.useState(
    workspaces[0] || initialValWorkspace,
  );

  React.useEffect(() => {
    if (activeWorkspaces.length > 0 && !activeWorkspaces.includes(workspace)) {
      setWorkspace(activeWorkspaces[0]);
    }
  }, [activeWorkspaces, workspace]);

  const joinMeeting = async (event) => {
    // prevent default to not reload page
    event.preventDefault();
    const data = new FormData(event.currentTarget);
    setWorkspace(data.get("workspace"));

    // await cleanupMeetingBeforeRejoin(connection.workspace);

    let response = "";
    let sender = connection.senderID;
    const run = async () => {
      try {
        if (sender) {
          let msg = "joinMeeting";
          let header = {
            user: connection.user,
          };
          console.log("Sending: " + JSON.stringify(msg) + " from " + sender);

          let buffer = new TextEncoder().encode(msg);
          response = await corelink.send(sender, buffer, header);
        }
      } catch (err) {
        console.log(err);
      }
    };

    updateConnection({ workspace: workspace });
    navigate("/controller");
  };

  const handleWSChange = (event) => {
    setWorkspace(event.target.value);
  };

  return (
    <div>
      <Paper elevation={3}>
        <Button
          onClick={handleOpen}
          // style={{width: "400px", height: "50px"}}
          fullWidth // make it stretch
          sx={{ height: 50 }} // optional: keep height
        >
          {label}
        </Button>
      </Paper>
      <Modal
        open={open}
        onClose={handleClose}
        aria-labelledby="modal-modal-title"
        aria-describedby="modal-modal-description"
      >
        <Box sx={style}>
          <Typography id="modal-modal-title" variant="h6" component="h2">
            {title}
          </Typography>
          <Typography id="modal-modal-description" sx={{ mt: 2 }}>
            {description}
          </Typography>
          <Box
            component="form"
            onSubmit={joinMeeting}
            noValidate
            sx={{ mt: 1 }}
          >
            <FormControl>
              <Select
                required
                fullWidth
                id="workspace"
                name="workspace"
                value={workspace}
                onChange={handleWSChange}
              >
                {workspaces.map((ws) => (
                  <MenuItem key={ws} value={ws}>
                    {ws} ({userCounts[ws] || 0} user
                    {userCounts[ws] !== 1 ? "s" : ""})
                  </MenuItem>
                ))}
              </Select>
              {/* <TextField
                    margin="normal"
                    fullWidth
                    id="meeting_password"
                    label="Password"
                    name="meeting_password"
                    autoFocus
                    /> */}
            </FormControl>
            {workspaces.length === 0 && (
              <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                No active meetings found. Create a meeting first.
              </Typography>
            )}
            <Button
              type="submit"
              fullWidth
              variant="contained"
              sx={{ mt: 3, mb: 2 }}
              disabled={loading || workspaces.length === 0}
            >
              Join Meeting!
            </Button>
          </Box>
        </Box>
      </Modal>
    </div>
  );
}
