import mqtt from 'mqtt';
import { TOPICS, SENSOR_WILDCARD } from './topics.js';

let client = null;
let machine = null;

export function connectMqtt(host, port, stateMachine) {
  machine = stateMachine;
  const brokerUrl = `mqtt://${host}:${port}`;

  client = mqtt.connect(brokerUrl, {
    clientId: `cupcake-backend-${Date.now()}`,
    reconnectPeriod: 3000,
    connectTimeout: 10000,
  });

  client.on('connect', () => {
    console.log(`[mqtt] Conectado al broker ${brokerUrl}`);
    client.subscribe([SENSOR_WILDCARD, TOPICS.OVERRIDE], (err) => {
      if (err) console.error('[mqtt] Error al suscribirse:', err.message);
    });
  });

  client.on('reconnect', () => console.log('[mqtt] Reconectando...'));
  client.on('error', (err) => console.error('[mqtt] Error:', err.message));
  client.on('close', () => console.log('[mqtt] Conexión cerrada'));

  client.on('message', (topic, payload) => {
    let data;
    try {
      data = JSON.parse(payload.toString());
    } catch {
      console.warn(`[mqtt] Payload inválido en ${topic}`);
      return;
    }

    if (topic.startsWith('cupcake/sensors/')) {
      machine.handleSensor(topic, data);
    } else if (topic === TOPICS.OVERRIDE) {
      machine.handleOverride(data);
    }
  });

  return client;
}

export function publish(topic, data) {
  if (!client?.connected) {
    console.warn(`[mqtt] No conectado, descartando mensaje en ${topic}`);
    return;
  }
  client.publish(topic, JSON.stringify(data), { retain: false });
}
