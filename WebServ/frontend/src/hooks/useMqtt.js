import { useState, useEffect, useRef, useCallback } from 'react';
import mqtt from 'mqtt';

const BROKER_WS_URL = 'ws://localhost:9001';

const TOPICS = {
  STATE:         'cupcake/state',
  HEARTBEAT:     'cupcake/sensors/heartbeat',
  CMD_ANIMATION: 'cupcake/commands/animation',
  CMD_SERVO:     'cupcake/commands/servo',
  OVERRIDE:      'cupcake/override',           // solo para cambios de estado de la mascota
};

export function useMqtt() {
  const [brokerConnected, setBrokerConnected] = useState(false);
  const [cupcakeState, setCupcakeState]       = useState(null);
  const [esp32Status, setEsp32Status]         = useState(null);
  const clientRef = useRef(null);

  useEffect(() => {
    const client = mqtt.connect(BROKER_WS_URL, {
      clientId: `cupcake-ui-${Date.now()}`,
      protocolVersion: 4,
      reconnectPeriod: 3000,
    });
    clientRef.current = client;

    client.on('connect', () => {
      setBrokerConnected(true);
      client.subscribe([TOPICS.STATE, TOPICS.HEARTBEAT]);
    });

    client.on('reconnect', () => setBrokerConnected(false));
    client.on('error',     () => setBrokerConnected(false));
    client.on('close',     () => setBrokerConnected(false));

    client.on('message', (topic, payload) => {
      try {
        const data = JSON.parse(payload.toString());
        if (topic === TOPICS.STATE)     setCupcakeState(data);
        if (topic === TOPICS.HEARTBEAT) setEsp32Status(data);
      } catch {
        // payload malformado
      }
    });

    return () => {
      client.end(true);
    };
  }, []);

  // Control manual directo → ESP32 (sin pasar por el backend)
  const sendAnimation = useCallback((id) => {
    clientRef.current?.publish(TOPICS.CMD_ANIMATION, JSON.stringify({ id }));
  }, []);

  const sendServo = useCallback((channel, angle) => {
    clientRef.current?.publish(TOPICS.CMD_SERVO, JSON.stringify({ channel, angle }));
  }, []);

  // Override de estado de la mascota → backend (para que gestione la máquina de estados)
  const sendStateOverride = useCallback((state) => {
    clientRef.current?.publish(TOPICS.OVERRIDE, JSON.stringify({ type: 'state', state }));
  }, []);

  return { brokerConnected, cupcakeState, esp32Status, sendAnimation, sendServo, sendStateOverride };
}
