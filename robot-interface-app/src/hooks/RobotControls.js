import React, {useState, useEffect, useRef, useCallback} from 'react';
import corelink from "../corelink.lib"
import {startPos, maxContraints, minContraints} from '../constants/motor_const';
import {useMeeting} from '../contexts/MeetingContext';

const useRobotControls = () => {
  const {controlSenderID} = useMeeting();

  const step = 1.5;
  const shiftedToNumber = {
    '!': 1,
    '@': 2,
    '#': 3,
    '$': 4,
    '%': 5
  };

  const prevInput = useRef({x: 0, y: 0});
  const input = useRef({x: 0, y: 0});

  const joystickInput = useRef({x:0, y:0}); // Keeps track of pos of the joystick component (-1 to 1)
  const [displayInput, setDisplayInput] = useState({x: 0, y: 0});

  const sliderValues = useRef(startPos);
  const sliderRef = useRef([null, null, null, null, null]);
  const sliderTextRef = useRef([null, null, null, null, null]);

  const [renderTick, setRenderTick] = useState(0);
  const forceRerender = () => setRenderTick(t => t + 1);

  // event --> IJoystickUpdateEvent
  const handleMove = (event) => {
    joystickInput.current.x = event.x;
    joystickInput.current.y = event.y;
  };
  const handleStop = (event) => {
    joystickInput.current.x = 0;
    joystickInput.current.y = 0;
  };
  const handleStart = (event) => {
    joystickInput.current.x = event.x;
    joystickInput.current.y = event.y;
  };

  const handleSliderChange = (sliderId, value) => {
    if (value < maxContraints[sliderId-1] && value > minContraints[sliderId-1]) {
      sliderValues.current[sliderId] = value;
    }
  };

  const setSliderValue = useCallback((i, val) => {
    const sliderElement = sliderRef.current[i];
    if (sliderElement) {
      const input = sliderElement.querySelector('input[type="range"]');
      if (input) {
        input.value = val;
        input.dispatchEvent(new Event('input', { bubbles: true }));
        input.dispatchEvent(new Event('change', { bubbles: true }));
      }
    } else {
      console.error(`Slider element with index ${i} not found`);
    }
    if (sliderTextRef.current[i]) {
      sliderTextRef.current[i].innerText = val.toFixed(1);
    }
  }, []);

  const handleKeyDown = useCallback((event) => {
    const lowerCaseKey = event.key.toLowerCase();
    // console.log("Key Pressed: ", event.key, " ", lowerCaseKey);

    if (event.key === 'ArrowUp' || lowerCaseKey === 'w') {
      input.current.y = 1;
    } else if (event.key === 'ArrowDown' || lowerCaseKey === 's') {
      input.current.y = -1;
    } else if (event.key === 'ArrowLeft' || lowerCaseKey === 'a') {
      input.current.x = -1;
    } else if (event.key === 'ArrowRight' || lowerCaseKey === 'd') {
      input.current.x = 1;
    } else if (['!', '2', '3', '4', '%'].includes(event.key)) {
      // if (['!', '%'].includes(event.key)) {
      //   const i = shiftedToNumber[event.key] - 1;
      // } else {
      //   const i = parseInt(event.key) - 1;
      // }
      const i = (['!', '%'].includes(event.key) ? shiftedToNumber[event.key] : parseInt(event.key)) - 1;
      if (sliderValues.current[i] + step <= maxContraints[i]) {
        sliderValues.current[i] = sliderValues.current[i] + step;
        setSliderValue(i, sliderValues.current[i]);
        corelink.send(controlSenderID, str2ab(JSON.stringify({controlType: "motor", id: i+1, pos: sliderValues.current[i]})), { type: 'robot-control' });
        forceRerender();
      }
    } else if (['1', '@', '#', '$', '5'].includes(event.key)) {
      // if (['1', '5'].includes(event.key)) {
      //   const i = parseInt(event.key) - 1;
      // } else {
      //   const i = shiftedToNumber[event.key] - 1;
      // }
      const i = (['1', '5'].includes(event.key) ? parseInt(event.key) - 1 : shiftedToNumber[event.key] - 1);
      if (sliderValues.current[i] - step >= minContraints[i]) {
        sliderValues.current[i] = sliderValues.current[i] - step;
        setSliderValue(i, sliderValues.current[i]);
        corelink.send(controlSenderID, str2ab(JSON.stringify({controlType: "motor", id: i+1, pos: sliderValues.current[i]})), { type: 'robot-control' });
        forceRerender();
      }
    }
  });

  useEffect(() => {
    for (let i = 0; i < sliderRef.current.length; i++) {
      setSliderValue(i, sliderValues.current[i]);
    }
    window.addEventListener('keydown', handleKeyDown);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    }
  }, [handleKeyDown]);

  useEffect(() =>{
    const interval = setInterval(() => {

      if (joystickInput.current.x || joystickInput.current.y) {
        // if we're using the joystick
        input.current.y = joystickInput.current.y;
        input.current.x = joystickInput.current.x;
      }

      if (controlSenderID) {
        if (!(prevInput.current.x === 0 && input.current.x === 0 && prevInput.current.y === 0 && input.current.y === 0)) {
          prevInput.current.x = input.current.x;
          prevInput.current.y = input.current.y;
          console.log("Sending: ", input.current.x, " ",input.current.y);
          corelink.send(controlSenderID, str2ab(JSON.stringify({ controlType: "nav" , x: input.current.x, y: input.current.y })), { type: 'robot-control' });
          input.current.x = 0;
          input.current.y = 0;
        }
      }
    }, 50);
    return () => clearInterval(interval);
  },[controlSenderID]);

  return {
    handleStart, handleMove, handleStop,
    handleSliderChange,
    sliderRef, sliderTextRef, sliderValues,
    sliderTextRef, renderTick,
    displayInput
  };
}

function str2ab(str) {
  const buf = new window.ArrayBuffer(str.length) // 2 bytes for each char
  const bufView = new window.Uint8Array(buf)
  for (let i = 0; i < str.length; i += 1) {
    bufView[i] = str.charCodeAt(i)
  }
  return buf
}

export default useRobotControls;