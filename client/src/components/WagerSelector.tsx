import { Dispatch, FC, SetStateAction } from "react";
import { Gambler } from "../types/serverUpdate";

const chipValues = [1, 5, 10, 25];

interface WagerSelectorProps {
  updateGambler: Dispatch<SetStateAction<Gambler>>;
}

const WagerSelector: FC<WagerSelectorProps> = ({ updateGambler }) => {
  const applyBet = (amount: number) => {
    updateGambler(prev => {
      let updatedBet = prev.bet;
      let updatedFunds = prev.funds;
      if (updatedFunds - amount >= 0) {
        updatedBet += amount;
        updatedFunds -= amount;
      }
      return { ...prev, funds: updatedFunds, bet: updatedBet };
    });
  };

  return (
    <div className="flex">
      {chipValues.map(val => {
        const chipImage = `/src/assets/chips/chip_${val}.png`;
        return (
          <div className="flex flex-col place-content-center m-2" key={val}>
            <div
              style={{ backgroundImage: `url(${chipImage})` }}
              className="bg-no-repeat bg-cover w-16 h-16 hover:scale-[1.05] cursor-pointer"
              onClick={() => applyBet(val)}
            ></div>
            <div className="text-center">{val}</div>
          </div>
        );
      })}
    </div>
  );
};

export default WagerSelector;
