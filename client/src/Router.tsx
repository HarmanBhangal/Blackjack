import { Route, Routes } from "react-router-dom";
import GameTable from "./pages/GameTable";
import { LandingPage } from "./pages/LandingPage";

export function Router() {
  return (
    <Routes>
      <Route path="/" element={<LandingPage />} />
      <Route path="/game" element={<GameTable />} />
    </Routes>
  );
}
