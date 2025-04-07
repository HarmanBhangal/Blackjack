import { FC } from "react";

interface RivalHandProps {
  cards: string[];
}

const RivalHand: FC<RivalHandProps> = ({ cards }) => {
  return (
    <div className="flex scale-50">
      {cards.map((cardStr, idx) => {
        const parts = cardStr.split("");
        const suit = parts.pop();
        const valueStr = parts.join("");
        const positionStyle = idx > 2 ? "absolute sendToAbove -bottom-32" : "";
        const imagePath = `/src/assets/cards/card${suit}.png`;
        return (
          <div
            key={idx}
            style={{ backgroundImage: `url(${imagePath})` }}
            className={`flex justify-center items-center text-6xl text-[#6D5C5C] w-[7rem] h-[11rem] bg-cover -ml-6 animate-getCard -rotate-2 ${positionStyle}`}
          >
            {valueStr}
          </div>
        );
      })}
    </div>
  );
};

export default RivalHand;
