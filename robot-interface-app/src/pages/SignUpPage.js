import * as React from "react";
import { useNavigate } from "react-router-dom";
import corelink from "../corelink.lib";
import Avatar from "@mui/material/Avatar";
import Button from "@mui/material/Button";
import CssBaseline from "@mui/material/CssBaseline";
import TextField from "@mui/material/TextField";
import FormControlLabel from "@mui/material/FormControlLabel";
import Checkbox from "@mui/material/Checkbox";
import Link from "@mui/material/Link";
import Grid from "@mui/material/Grid";
import Box from "@mui/material/Box";
import LockOutlinedIcon from "@mui/icons-material/LockOutlined";
import Typography from "@mui/material/Typography";
import Container from "@mui/material/Container";
import Snackbar from "@mui/material/Snackbar";
import IconButton from "@mui/material/IconButton";
import CloseIcon from "@mui/icons-material/Close";

function Copyright(props) {
  return (
    <Typography
      variant="body2"
      color="text.secondary"
      align="center"
      {...props}
    >
      {"Copyright © "}
      <Link color="inherit" href="https://mui.com/">
        Your Website
      </Link>{" "}
      {new Date().getFullYear()}
      {"."}
    </Typography>
  );
}

// TODO remove, this demo shouldn't need to reset the theme.

const requiredFields = ["username", "password", "email"];

export default function SignUpPage() {
  const navigate = useNavigate();
  const [open, setOpen] = React.useState(false);
  const [error, setError] = React.useState("");
  const vertical = "top";
  const horizontal = "center";

  const handleSubmit = async (event) => {
    event.preventDefault();
    const data = new FormData(event.currentTarget);
    for (const field of data.entries()) {
      const [fieldname, fieldvalue] = field;
      if (requiredFields.includes(fieldname) && fieldvalue == "") {
        console.log("NULL FIELD VALUE");
        setError("Required field values can not be null.");
        setOpen(true);
        return;
      }
    }
    let options = {
      username: data.get("username"),
      password: data.get("password"),
    };
    if (data.get("firstName")) options.first = data.get("firstName");
    if (data.get("lastName")) options.last = data.get("lastName");
    if (data.get("email")) options.email = data.get("email");

    try {
      // This is bad -- credentials should be passed in by the user,
      // Elevated privileges need to be used to add users
      let credentials = {
        username: "",
        password: "",
      };
      let configs = {
        ControlIP: "127.0.0.1",
        ControlPort: 20012,
      };
      await corelink.connect(credentials, configs);

      await corelink.addUser(options);
      navigate("/");
    } catch (error) {
      console.log(error);
      setError("Not authorized to create account!");
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
        <CloseIcon fontSize="small" />
      </IconButton>
    </React.Fragment>
  );

  return (
    <Container
      component="main"
      maxWidth="xs"
      sx={{
        bgcolor: "#fafafa", // light background (optional)
        p: 4, // padding around the form
        borderRadius: 3, // rounded corners
        boxShadow: 3, // subtle shadow
        mt: 8, // margin top
      }}
    >
      <CssBaseline />
      <Box
        sx={{
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          p: 2,
        }}
      >
        <Avatar sx={{ m: 1, bgcolor: "secondary.main" }}>
          <LockOutlinedIcon />
        </Avatar>
        <Typography component="h1" variant="h5">
          Sign Up for Data Center Robot
        </Typography>
        <Box component="form" noValidate onSubmit={handleSubmit} sx={{ mt: 3 }}>
          <Grid container spacing={2}>
            <Grid item xs={12} sm={6}>
              <TextField
                autoComplete="given-name"
                name="firstName"
                fullWidth
                id="firstName"
                label="First Name"
                autoFocus
              />
            </Grid>
            <Grid item xs={12} sm={6}>
              <TextField
                fullWidth
                id="lastName"
                label="Last Name"
                name="lastName"
                autoComplete="family-name"
              />
            </Grid>
            <Grid item xs={12}>
              <TextField
                fullWidth
                required
                id="email"
                label="Email Address"
                name="email"
                autoComplete="email"
              />
            </Grid>
            <Grid item xs={12}>
              <TextField
                margin="normal"
                required
                fullWidth
                id="username"
                label="Username"
                name="username"
                autoComplete="Username"
                autoFocus
              />
            </Grid>
            <Grid item xs={12}>
              <TextField
                margin="normal"
                required
                fullWidth
                name="password"
                label="Password"
                type="password"
                id="password"
                autoComplete="current-password"
              />
            </Grid>
          </Grid>
          <Button
            type="submit"
            fullWidth
            variant="contained"
            sx={{ mt: 3, mb: 2 }}
          >
            Sign Up
          </Button>
          <Grid container>
            <Grid item>
              <Link href="/" variant="body2">
                Already have an account? Sign in
              </Link>
            </Grid>
          </Grid>
          <Snackbar
            anchorOrigin={{ vertical, horizontal }}
            open={open}
            autoHideDuration={5000}
            onClose={handleClose}
            message={error}
            action={action}
          />
        </Box>
      </Box>
      <Copyright sx={{ mt: 5 }} />
    </Container>
  );
}
