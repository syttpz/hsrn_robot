import * as React from "react";
import { Box, Button, Select, MenuItem, FormControl } from "@mui/material";
// import Button from '@mui/material/Button';
import Typography from "@mui/material/Typography";
import Modal from "@mui/material/Modal";
// import TextField from '@mui/material/TextField';
import Paper from "@mui/material/Paper";
import corelink from "../../corelink.lib";
import { useNavigate } from "react-router-dom";
// import { useEffect } from 'react';
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

export default function CreateMeetingModal({
  label,
  title,
  description,
  connection,
  updateConnection,
  availableWorkspaces = [],
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
  const workspaces =
    availableWorkspaces.length > 0 ? availableWorkspaces : MEETING_WORKSPACES;
  const [workspace, setWorkspace] = React.useState(
    workspaces[0] || initialValWorkspace,
  );

  React.useEffect(() => {
    if (
      availableWorkspaces.length > 0 &&
      !availableWorkspaces.includes(workspace)
    ) {
      setWorkspace(availableWorkspaces[0]);
    }
  }, [availableWorkspaces, workspace]);

  const createMeeting = async (event) => {
    // prevent default to not reload page
    event.preventDefault();
    const data = new FormData(event.currentTarget);
    let users = [connection.user];
    let host = connection.user;
    setWorkspace(data.get("workspace"));

    // await cleanupMeetingBeforeRejoin(connection.workspace);

    /** HANDSHAKE START **/

    let response = "";
    let sender = connection.senderID;

    // const run = async () => {
    //   try {
    //     if (sender) {
    //       let msg = "createMeeting";
    //       let header = {
    //         user: connection.user
    //       }
    //       console.log("Sending: " + JSON.stringify(msg) + " from " + sender);

    //       let buffer = new TextEncoder().encode(msg);
    //       response = await corelink.send(sender, buffer, header);
    //     }
    //   } catch (err) {
    //     console.log(err);
    //   }
    // }

    // if (connection.user) {  // Connected to client
    //   run();
    // }

    let statemgmt_stream = await corelink
      .listStreams({ workspaces: connection.smWorkspace, types: "statemgmt" })
      .catch((err) => {
        console.log(err);
      });
    if (statemgmt_stream.senderList.length === 0) {
      console.log("Statemgmt not found.");
    } else {
      console.log(
        "Statemgmt Stream: " + JSON.stringify(statemgmt_stream.senderList),
      ); // streamID
    }

    /** HANDSHAKE END **/
    updateConnection({ workspace: workspace });
    // navigate to controller
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
            onSubmit={createMeeting}
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
                    {ws}
                  </MenuItem>
                ))}
              </Select>
            </FormControl>
            {workspaces.length === 0 && (
              <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                All workspaces have active meetings. Join an existing meeting
                instead.
              </Typography>
            )}
            <Button
              type="submit"
              fullWidth
              variant="contained"
              sx={{ mt: 3, mb: 2 }}
              disabled={loading || workspaces.length === 0}
            >
              Create Meeting!
            </Button>
          </Box>
        </Box>
      </Modal>
    </div>
  );
}
