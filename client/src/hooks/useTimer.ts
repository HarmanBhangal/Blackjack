import { useEffect, useRef } from "react";

export const useTimer = (fn: () => void, interval: number) => {
  const savedFn = useRef(fn);

  useEffect(() => {
    savedFn.current = fn;
  }, [fn]);

  useEffect(() => {
    const tick = () => {
      savedFn.current();
    };
    if (interval !== null) {
      const timerId = setInterval(tick, interval);
      return () => clearInterval(timerId);
    }
  }, [interval]);
};
