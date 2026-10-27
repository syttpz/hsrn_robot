import React, { createContext, useState, useRef, useContext } from "react";

// MASTER CONTEXT
const MeetingContext = createContext();

export const MeetingProvider = ({ children }) => {
  const [inMeeting, setInMeeting] = useState(false);
  const [videoSenderID, setVideoSenderID] = useState(null);
  const [audioSenderID, setAudioSenderID] = useState(null);
  const [controlSenderID, setControlSenderID] = useState(null);
  const [videoStreamList, setVideoStreamList] = useState([]);
  const [audioStreamList, setAudioStreamList] = useState([]);
  const [initAV, setInitAV] = useState(false);

  return (
    <MeetingContext.Provider
      value={{
        inMeeting,
        setInMeeting,
        videoSenderID,
        setVideoSenderID,
        audioSenderID,
        setAudioSenderID,
        controlSenderID,
        setControlSenderID,
        videoStreamList,
        setVideoStreamList,
        audioStreamList,
        setAudioStreamList,
        initAV,
        setInitAV,
      }}
    >
      {children}
    </MeetingContext.Provider>
  );
};

export const useMeeting = () => useContext(MeetingContext);
