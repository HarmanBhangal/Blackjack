import { FC } from "react";

interface DecisionControlsProps {
  processChoice: (choice: string) => void;
  mode: string;
  isEnabled?: boolean;
}

const DecisionControls: FC<DecisionControlsProps> = ({
  processChoice,
  mode,
  isEnabled = false,
}) => {
  return (
    <div>
      {mode === "WAGERING" ? (
        <button
          onClick={() => processChoice("CLEAR")}
          className="text-3xl w-44 h-14 rounded-md bg-primary text-bg transition-colors"
        >
          clear
        </button>
      ) : (
        <>
          <button
            onClick={() => processChoice("HIT")}
            disabled={!isEnabled}
            className={`text-3xl w-44 h-14 rounded-md ${isEnabled ? "bg-primary text-bg" : "bg-shadow text-secondary"} transition-colors`}
          >
            hit
          </button>
          <button
            onClick={() => processChoice("STAND")}
            disabled={!isEnabled}
            className={`text-3xl w-20 h-14 ${isEnabled ? "bg-b-secondary text-bg" : "bg-shadow text-secondary"} transition-colors rounded-md ml-4`}
          >
            stay
          </button>
        </>
      )}
    </div>
  );
};

export default DecisionControls;
