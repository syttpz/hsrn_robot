// import ReactDOM from "react-dom/client";
import "./styles/var.css";
import theme from "./constants/theme";
import { ThemeProvider } from "@mui/material/styles";
import { BrowserRouter, Routes, Route } from "react-router-dom";
import { useState, useEffect } from "react";
import ControllerPage from "./pages/ControllerPage";
import SignInPage from "./pages/SignInPage";
import SignUpPage from "./pages/SignUpPage";
import PreLandingPage from "./pages/PreLandingPage";
import LandingPage from "./pages/LandingPage";
import NoPage from "./pages/NoPage";

import { SM_WORKSPACE } from "./constants/corelink_const";
import { reconnectCorelink } from "./utils/corelinkUtils";

function App() {
  // Load connection from localStorage if available
  const loadConnection = () => {
    try {
      const saved = localStorage.getItem("robotConnection");
      if (saved) {
        return JSON.parse(saved);
      }
    } catch (error) {
      console.error("Error loading connection from localStorage:", error);
    }
    // Default connection if nothing saved
    return {
      user: "Test",
      smWorkspace: SM_WORKSPACE, // state management workspace
      smSenderID: "",
      workspace: "",
    };
  };

  // Connection will hold relevant info that can be shared between pages
  const [connection, setConnection] = useState(loadConnection());
  const [corelinkReady, setCorelinkReady] = useState(false);

  // Save connection to localStorage whenever it changes
  useEffect(() => {
    try {
      localStorage.setItem("robotConnection", JSON.stringify(connection));
    } catch (error) {
      console.error("Error saving connection to localStorage:", error);
    }
  }, [connection]);

  // On page load/refresh: reconnect corelink from saved credentials (only on authenticated pages, skip on login page)
  useEffect(() => {
    if (window.location.pathname === "/") {
      setCorelinkReady(true);
      return;
    }
    reconnectCorelink()
      .then((result) => {
        console.log("[App] Corelink reconnect result:", result);
      })
      .finally(() => {
        setCorelinkReady(true);
      });
  }, []);

  // Function to update connection (makes changes to the attributes of connection you want to update, leaves rest the same)
  const updateConnection = (newConn) => {
    setConnection((conn) => ({
      ...conn,
      ...newConn,
    }));
  };

  return (
    // Routing, default page is sign in page
    // Note: Reloading is buggy, TODO
    <BrowserRouter>
      <ThemeProvider theme={theme}>
        {!corelinkReady ? null : (
          <Routes>
            <Route
              path="/"
              element={
                <SignInPage
                  connection={connection}
                  updateConnection={updateConnection}
                />
              }
            />
            {/* <Route path = "/signup" element={<SignUpPage connection={connection} updateConnection={updateConnection}/>} /> */}
            <Route
              path="/prelanding"
              element={
                <PreLandingPage
                  connection={connection}
                  updateConnection={updateConnection}
                />
              }
            />
            <Route
              path="/landing"
              element={
                <LandingPage
                  connection={connection}
                  updateConnection={updateConnection}
                />
              }
            />
            <Route
              path="/controller"
              element={
                <ControllerPage
                  connection={connection}
                  updateConnection={updateConnection}
                />
              }
            />
            <Route path="*" element={<NoPage />} />
          </Routes>
        )}
      </ThemeProvider>
    </BrowserRouter>
  );
}

export default App;
