import { useCallback } from "react";
import { VIDEO_W, VIDEO_H } from "../constants/corelink_const";

const useCanvas = () => {
  const VIDEO_WPX = `${VIDEO_W}px`;
  const VIDEO_HPX = `${VIDEO_H}px`;
  const colorA = "#f9c7f4";
  const colorB = "#a4a1a7";
  const ratio = window.devicePixelRatio || 1;

  const initContext = useCallback((canvas) => {
    const ctx = canvas.getContext("2d", { willReadFrequently: true });
    canvas.width = VIDEO_W * ratio;
    canvas.height = VIDEO_H * ratio;
    canvas.style.width = VIDEO_WPX;
    canvas.style.height = VIDEO_HPX;
    ctx.scale(ratio, ratio);
    return ctx;
  });

  const defaultCanvas = useCallback((username, ctx, color) => {
    ctx.fillStyle = color;
    ctx.fillRect(0, 0, VIDEO_W, VIDEO_H);
    ctx.fillStyle = "#000000";
    ctx.font = "30px Verdana";
    ctx.textBaseline = "middle";
    ctx.textAlign = "center";
    ctx.fillText(username, VIDEO_W / 2, VIDEO_H / 2);
  });

  const setNameCanvasA = useCallback((username, ctx) => {
    defaultCanvas(username, ctx, colorA);
  });

  const setNameCanvasB = useCallback((username, ctx) => {
    defaultCanvas(username, ctx, colorB);
  });

  return { initContext, setNameCanvasA, setNameCanvasB };
};

export default useCanvas;
