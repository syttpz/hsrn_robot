import React, { useEffect, useState, useRef } from 'react';
import corelink from '../corelink.lib';
import { useNavigate } from 'react-router-dom';
import { Box, CircularProgress } from '@mui/material';

export default function PreLandingPage({ connection, updateConnection }) {
  const [senderID, setSenderID] = useState(connection.smSenderID);
  const startedInitRef = useRef(false); // Use ref instead of state to prevent re-renders
  const hasNavigatedRef = useRef(false); // Prevent double navigation
  const navigate = useNavigate();

  useEffect(() => {
    // Prevent double execution in React Strict Mode
    if (hasNavigatedRef.current) return;

    const initializePage = async () => {
      // Navigation guard: redirect if no user or workspace
      if (!connection.user || !connection.smWorkspace) {
        console.log('[PreLandingPage] Missing user or workspace, redirecting to sign in');
        hasNavigatedRef.current = true;
        navigate('/');
        return;
      }

      // Check if sender already exists in localStorage or connection
      const savedSenderID = localStorage.getItem('smSenderID');
      if (connection.smSenderID || savedSenderID) {
        const existingSenderID = connection.smSenderID || savedSenderID;
        console.log('[PreLandingPage] Sender already exists:', existingSenderID);
        if (savedSenderID && !connection.smSenderID) {
          updateConnection({ smSenderID: savedSenderID });
        }
        hasNavigatedRef.current = true;
        navigate('/landing');
        return;
      }

      // Only create sender if not already started
      if (startedInitRef.current) return;
      startedInitRef.current = true;

      console.log('[PreLandingPage] Creating states sender for user: ' + connection.user);

      // Skip reconnect — connection is alive from sign-in.
      // Calling reconnectCorelink() replaces the WebSocket but doesn't re-login
      // (token already set), breaking server event delivery.

      const corelink_consts = {
        tcp_control_port: 20010,
        ws_control_port: 20012,
        ws_data_port: 20013,
      };
      const protocol = 'ws'; // only option

      try {
        let sender_metadata = 'User';
        let sender = await corelink.createSender({
          workspace: connection.smWorkspace,
          type: 'statemgmt',
          protocol,
          port: corelink_consts.ws_data_port,
          metadata: sender_metadata,
        });
        setSenderID(sender);
        console.log('[PreLandingPage] SenderID created: ' + sender);

        // Store sender ID in localStorage
        try {
          localStorage.setItem('smSenderID', sender);
          console.log('[PreLandingPage] Sender ID saved to localStorage');
        } catch (storageError) {
          console.error('[PreLandingPage] Error saving sender ID to localStorage:', storageError);
        }

        corelink.on(sender, (data, header) => {
          console.log('[PreLandingPage] Received: ' + data + ' from ' + header.user);
        });
      } catch (err) {
        console.error('[PreLandingPage] Error creating sender:', err);
        startedInitRef.current = false;
        return;
      }

      try {
        let response = '';
        const sender = connection.sender;
        if (sender) {
          let msg = 'userOnline';
          let header = {
            user: connection.user,
          };
          console.log('[PreLandingPage] Sending: ' + JSON.stringify(msg) + ' from ' + sender);

          let buffer = new TextEncoder().encode(msg);
          await corelink.send(sender, buffer, header);
        }
      } catch (err) {
        console.error('[PreLandingPage] Error sending user online message:', err);
      }
    };

    initializePage();
  }, []); // Empty dependency array - only run once

  useEffect(() => {
    if (senderID && !hasNavigatedRef.current) {
      console.log('[PreLandingPage] Updating connection with sender ID and navigating to landing');
      updateConnection({ smSenderID: senderID });
      hasNavigatedRef.current = true;
      navigate('/landing');
    }
  }, [senderID, navigate, updateConnection]);

  return (
    <Box
      sx={{
        display: 'flex',
        justifyContent: 'center',
        alignItems: 'center',
        height: '100vh',
      }}
    >
      <CircularProgress />
    </Box>
  );
}
