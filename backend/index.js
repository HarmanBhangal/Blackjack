import cors from 'cors';
import express from 'express';
import net from 'net';
import bodyParser from 'body-parser';

const CLIENT_INACTIVITY_LIMIT = 20; // seconds
const GAME_SERVER_PORT = parseInt(process.argv[2] ?? 2000);
const clientConnections = {};

const apiApp = express();
apiApp.use(cors());
apiApp.use(bodyParser.json());

function establishServerLink() {
  return new Promise((resolve, reject) => {
    const gameSocket = new net.Socket();
    gameSocket.connect(GAME_SERVER_PORT, 'localhost', () => {
      console.log('✅ Connected to C++ game server on port', GAME_SERVER_PORT);
    });
    gameSocket.once('data', (incoming) => {
      const initData = incoming.toString();
      console.log("📦 Initial server data received:\n", initData);
      resolve([gameSocket, initData]);
    });
    gameSocket.on('error', (err) => {
      console.error('❌ Game server connection error:', err.message);
      reject(err);
    });
  });
}

apiApp.get("/connect", async (req, res) => {
  console.log("🔗 New client attempting to connect...");
  try {
    const [gameSocket, initMsg] = await establishServerLink();
    const initialData = JSON.parse(initMsg);
    const clientId = initialData["playerID"];
    const currentGameState = initialData["gameState"];
    if (!clientId) return res.status(500).send({ error: "Invalid game server response" });

    monitorGameSocket(gameSocket, clientId);
    clientConnections[clientId] = {
      socket: gameSocket,
      data: currentGameState,
      lastActive: Date.now()
    };

    console.log(`✅ Player ${clientId} connected.`);
    res.send(JSON.stringify({ id: clientId }));
  } catch (err) {
    console.error("❌ Connection failed:", err);
    res.status(500).send({ error: "Unable to connect to game server" });
  }
});

apiApp.get('/update/:id', (req, res) => {
  const client = clientConnections[req.params.id];
  if (client) {
    client.lastActive = Date.now();
    res.send(client.data ?? {});
  } else {
    res.status(404).send({ error: "Client not connected" });
  }
});

apiApp.post('/action/:id', (req, res) => {
  const client = clientConnections[req.params.id];
  if (!client) return res.status(404).send({ error: "Client not connected" });
  const payload = JSON.stringify(req.body);
  console.log(`📨 Client ${req.params.id} sent action:`, payload);
  client.socket.write(Buffer.from(payload));
  res.send({ status: "Action forwarded to game server" });
});

apiApp.listen(3000, () => {
  console.log('\n🚀 Middleware API running on http://localhost:3000');
  console.log(`🌐 Forwarding requests to C++ server on port ${GAME_SERVER_PORT}`);
});

function monitorGameSocket(socket, id) {
  socket.on('data', (incoming) => {
    if (clientConnections[id]) {
      clientConnections[id].data = incoming.toString();
    }
  });
  socket.on('close', () => {
    console.log(`🔌 Connection closed for player ${id}`);
    delete clientConnections[id];
  });
  socket.on('error', (err) => {
    console.error(`❗ Socket error for player ${id}:`, err.message);
  });
}

setInterval(() => {
  const now = Date.now();
  for (const id in clientConnections) {
    if (clientConnections[id].lastActive < now - CLIENT_INACTIVITY_LIMIT * 1000) {
      clientConnections[id].socket.end();
      console.log(`⏳ Disconnected inactive client #${id}`);
      delete clientConnections[id];
    }
  }
}, 5000);
