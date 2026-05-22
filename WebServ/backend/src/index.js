import 'dotenv/config';
import express from 'express';
import cors from 'cors';
import { CupcakeMachine } from './state/machine.js';
import { connectMqtt } from './mqtt/client.js';
import { startScheduler } from './behaviors/scheduler.js';
import { createRouter } from './routes/api.js';

const PORT      = process.env.PORT      ?? 3001;
const MQTT_HOST = process.env.MQTT_HOST ?? 'localhost';
const MQTT_PORT = process.env.MQTT_PORT ?? 1883;

// 1. Máquina de estados (cerebro)
const machine = new CupcakeMachine();

// 2. Conexión MQTT (nervios)
connectMqtt(MQTT_HOST, MQTT_PORT, machine);

// 3. Comportamientos autónomos
startScheduler(machine);

// 4. Servidor Express (API de diagnóstico)
const app = express();
app.use(cors());
app.use(express.json());
app.use('/api', createRouter(machine));

app.get('/health', (_req, res) => res.json({ ok: true, service: 'cupcake-backend' }));

app.listen(PORT, () => {
  console.log(`[server] Backend escuchando en http://localhost:${PORT}`);
  console.log(`[server] Broker MQTT → ${MQTT_HOST}:${MQTT_PORT}`);
});
