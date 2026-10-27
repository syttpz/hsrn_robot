import React, {useState, useCallback} from 'react';
import Snackbar from "@mui/material/Snackbar";

const useSnackbar = () => {
  const [snackbarMessage, setSnackbarMessage] = useState('');
  const [isSnackbarOpen, setIsSnackbarOpen] = useState(false);

  const showSnackbar = useCallback((message) => {
    setSnackbarMessage(message);
    setIsSnackbarOpen(true);
  }, []);

  const handleSnackbarClose = useCallback(() => {
    setIsSnackbarOpen(false);
  }, []);

  const SnackbarComponent = (
    <Snackbar
      anchorOrigin={{vertical: 'bottom', horizontal: 'center'}}
      open={isSnackbarOpen}
      message={snackbarMessage}
      autoHideDuration={3000}
      onClose={handleSnackbarClose}
    />
  );

  return {
    snackbarMessage,
    isSnackbarOpen,
    showSnackbar,
    handleSnackbarClose,
    SnackbarComponent
  };
};

export default useSnackbar;
