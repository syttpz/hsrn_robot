import React from "react";
import { useNavigate } from "react-router-dom";
import corelink from "../corelink.lib";
import { saveCorelinkCredentials } from "../utils/corelinkUtils";

// MUI Imports
import {
  LockOutlined,
  Close,
  Visibility,
  VisibilityOff,
} from "@mui/icons-material/";
import Button from "@mui/material/Button";
import TextField from "@mui/material/TextField";
import Box from "@mui/material/Box";
import Typography from "@mui/material/Typography";
import Container from "@mui/material/Container";
import Snackbar from "@mui/material/Snackbar";
import IconButton from "@mui/material/IconButton";

export default function SignInPage({ connection, updateConnection }) {
  const [open, setOpen] = React.useState(false);
  const [error, setError] = React.useState("");
  const [showPassword, setShowPassword] = React.useState(false);

  const vertical = "top";
  const horizontal = "center";
  const navigate = useNavigate();

  const handleSubmit = async (event) => {
    event.preventDefault();
    setShowPassword(false);
    const data = new FormData(event.currentTarget);
    for (const field of data.entries()) {
      const [fieldname, fieldvalue] = field;
      if (typeof fieldvalue === "string" && fieldvalue.trim() === "") {
        console.log("NULL FIELD VALUE");
        setError("All fields are required.");
        setOpen(true);
        return;
      }
    }

    let credentials = {
      username: data.get("username"),
      password: data.get("password"),
    };
    let configs = {
      ControlIP: data.get("host"),
      ControlPort: 20012,
    };
    try {
      await corelink.connect(credentials, configs);

      // Store credentials in localStorage for reconnection on page reload (password is encrypted)
      await saveCorelinkCredentials(credentials, configs);

      let updatedUser = { user: data.get("username") };
      updateConnection(updatedUser);
      navigate("/prelanding");
    } catch (error) {
      console.log("ERROR caught:", error);
      if (error.currentTarget instanceof WebSocket) {
        setError("Error logging in: Invalid host.");
      } else {
        setError("Error logging in: Please check your credentials.");
      }
      setOpen(true);
    }
    return;
  };

  const handleClose = (event, reason) => {
    if (reason === "clickaway") {
      return;
    }

    setOpen(false);
  };

  const action = (
    <React.Fragment>
      <IconButton
        size="small"
        aria-label="close"
        color="inherit"
        onClick={handleClose}
      >
        <Close fontSize="small" />
      </IconButton>
    </React.Fragment>
  );

  return (
    <Box
      sx={{
        position: "fixed",
        height: "100vh",
        width: "100vw",
        boxSizing: "border-box",
        backgroundImage:
          'url("https://www.nyu.edu/content/dam/nyu/social/images/WALLPAPERS/2020_0706_nyu-background_2_3x2.jpg")',
        backgroundSize: "cover",
        backgroundPosition: "center",
      }}
    >
      <Box
        sx={{
          position: "absolute",
          top: 0,
          left: 0,
          width: "100%",
          height: "100%",
          boxSizing: "border-box",
          zIndex: -1,
          backgroundColor: "rgba(0,0,0,0.4)",
        }}
      />
      <Container
        maxWidth="xs"
        sx={{
          mt: 8,
          p: 4,
          borderRadius: 3,
          boxShadow: 3,
          backgroundColor: "var(--light)",
        }}
      >
        {/* Login Box */}
        <Box
          sx={{
            position: "relative",
            display: "flex",
            flexDirection: "column",
            alignItems: "center",
            p: 2,
            gap: 2,
          }}
        >
          <img
            style={{
              bgcolor: "var(--accent-color)",
              maxWidth: "100%",
              maxHeight: 30,
            }}
            src="/hsrn.png"
            alt="HSRN Logo"
          />
          <Typography
            textAlign="center"
            fontSize={20}
            sx={{ color: "var(--text-main)", mb: 2 }}
          >
            Data Center Robot Interface
          </Typography>

          <Box component="form" onSubmit={handleSubmit} noValidate>
            <TextField
              margin="normal"
              required
              fullWidth
              id="username"
              label="Username"
              name="username"
              autoComplete="Username"
              defaultValue="Testuser"
              autoFocus
              InputLabelProps={{ style: { color: "var(--text-main)" } }}
              InputProps={{ style: { color: "var(--text-main)" } }}
            />
            <Box sx={{ position: "relative" }}>
              <TextField
                margin="normal"
                required
                fullWidth
                name="password"
                label="Password"
                type={showPassword ? "text" : "password"}
                id="password"
                autoComplete="current-password"
                defaultValue="Testpassword"
                InputLabelProps={{ style: { color: "var(--text-main)" } }}
                InputProps={{ style: { color: "var(--text-main)" } }}
              />
              <Button
                variant="text"
                sx={{
                  position: "absolute",
                  right: 8,
                  top: "calc(50% - 12px)",
                  minWidth: 0,
                  minHeight: 0,
                  padding: 0,
                  width: 32,
                  height: 32,
                  borderRadius: "50%",
                  display: { xs: "none", sm: "inline-flex" },
                }}
                onClick={(e) => {
                  e.stopPropagation();
                  setShowPassword((prev) => !prev);
                }}
              >
                {showPassword ? (
                  <VisibilityOff sx={{ m: 0, color: "var(--text-muted)" }} />
                ) : (
                  <Visibility sx={{ m: 0, color: "var(--text-muted)" }} />
                )}
              </Button>
            </Box>
            <TextField
              margin="normal"
              required
              fullWidth
              name="host"
              label="Host"
              type="text"
              id="Host"
              autoComplete="current-host"
              defaultValue="127.0.0.1"
              InputLabelProps={{ style: { color: "var(--text-main)" } }}
              InputProps={{ style: { color: "var(--text-main)" } }}
            />
            <Button
              type="submit"
              fullWidth
              variant="contained"
              sx={{
                mt: 3,
                mb: 2,
                bgcolor: "var(--accent-color)",
                color: "var(--on-accent)",
              }}
            >
              Sign In
            </Button>
            <Snackbar
              anchorOrigin={{ vertical, horizontal }}
              open={open}
              autoHideDuration={3000}
              onClose={handleClose}
              message={error}
              action={action}
              ContentProps={{
                sx: {
                  backgroundColor: "var(--transparent-white)",
                  color: "var(--alert)",
                },
              }}
            />
          </Box>
        </Box>
      </Container>
    </Box>
  );
}
