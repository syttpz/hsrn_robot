import { createTheme } from "@mui/material/styles";
// Import CSS file for variables
import "../styles/var.css";

// Helper to get CSS variable value
const getCssVar = (name) =>
  getComputedStyle(document.documentElement).getPropertyValue(name).trim();

const theme = createTheme({
  palette: {
    background: {
      default: getCssVar("--bg-body"),
      paper: getCssVar("--light"),
    },
    primary: {
      main: getCssVar("--primary-color"),
      contrastText: getCssVar("--on-accent"),
    },
    secondary: {
      main: getCssVar("--secondary-color"),
      contrastText: getCssVar("--light"),
    },
    tertiary: {
      main: getCssVar("--tertiary-color"),
      contrastText: getCssVar("--light"),
    },
    accent: {
      main: getCssVar("--accent-color"),
      contrastText: getCssVar("--on-accent"),
    },
    muted: {
      main: getCssVar("--muted"),
      contrastText: getCssVar("--text-dark"),
    },
    text: {
      primary: getCssVar("--text-main"),
      secondary: getCssVar("--text-muted"),
      disabled: getCssVar("--muted"),
      dark: getCssVar("--text-dark"),
      bright: getCssVar("--text-bright"),
    },
    transparentBg: {
      main: getCssVar("--transparent-bg"),
    },
  },
});

export default theme;
