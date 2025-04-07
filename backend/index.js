// File: index.js

import cors from 'cors';
import express from 'express';
import net from 'net';
import bodyParser from 'body-parser';

// Configuration constants
const CLIENT_INACTIVITY_LIMIT = 20; // seconds until a client is considered disconnected
const GAME_SERVER_PORT = parseInt(process.argv[2] ?? 2000);

// Object to track active client connections
const clientConnections = {};

// Initialize Express app
const apiApp = express();
apiApp.use(cors());
apiApp.use(bodyParser.json());

// Establish connection to the game server and return initial data
function establishServerLink() {
  return new Promise((resolve, reject) => {
    const gameSocket = new net.Socket();

    gameSocket.connect(GAME_SERVER_PORT, 'localhost', () => {
      console.log('Connected to game server');
    });

    gameSocket.on('data', (incoming) => {
      resolve([gameSocket, incoming.toString()]);
    });

    gameSocket.on('error', (err) => {
      reject(err);
    });
  });
}

// Endpoint: /connect
apiApp.get("/connect", (req, res) => {
  console.log("New client connection request received...");
  establishServerLink()
    .then(([gameSocket, initMsg]) => {
      const initialData = JSON.parse(initMsg);
      const clientId = initialData["playerID"];
      const currentGameState = initialData["gameState"];

      // Begin monitoring this game socket
      monitorGameSocket(gameSocket, clientId);

      // Save the client connection data
      clientConnections[clientId] = {
        socket: gameSocket,
        data: currentGameState,
        lastActive: Date.now()
      };

      // Return client ID to the caller
      res.send(JSON.stringify({ id: clientId }));
    })
    .catch((error) => {
      console.error(error);
    });
});

// Endpoint: /update/:id – used for polling game state updates
apiApp.get('/update/:id', (req, res) => {
  console.log(`Received update poll from client ${req.params.id}`);
  clientConnections[req.params.id].lastActive = Date.now();
  res.send(clientConnections[req.params.id]?.data ?? {});
});

// Endpoint: /action/:id – used for client actions (BET, TURN)
apiApp.post('/action/:id', (req, res) => {
  console.log(`Client ${req.params.id} submitted an action`);
  const targetSocket = clientConnections[req.params.id].socket;
  targetSocket.write(Buffer.from(JSON.stringify(req.body)));
});

// Start the API server on port 3000
apiApp.listen(3000, () => {
  console.log('Backend API is running on port 3000');
  console.log(`All requests will be routed to game server on port ${GAME_SERVER_PORT}`);
});

// Function to monitor a game socket for data, close, and errors
function monitorGameSocket(socket, id) {
  socket.on('data', (incoming) => {
    if (clientConnections[id]) {
      clientConnections[id].data = incoming.toString();
    }
  });

  socket.on('close', () => {
    console.log('Game server connection closed');
    Object.keys(clientConnections).forEach((connId) => {
      clientConnections[connId].data = { status: 3 };
    });
  });

  socket.on('error', (err) => {
    console.error(`Error on socket for client ${id}: ${err}`);
  });
}

// Periodically check and disconnect inactive client connections
function disconnectInactiveClients() {
  for (let id in clientConnections) {
    if (clientConnections[id].lastActive < Date.now() - CLIENT_INACTIVITY_LIMIT * 1000) {
      clientConnections[id].socket.end();
      console.log("Disconnected client #" + id);
      delete clientConnections[id];
    }
  }
}

setInterval(disconnectInactiveClients, 5000);
