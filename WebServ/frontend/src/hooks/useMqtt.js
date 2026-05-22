import { useState, useEffect, useRef, useCallback } from 'react';
import mqtt from 'mqtt';

const BROKER_WS_URL = 'ws://localhost:9001';

const TOPICS = {
  STATE:     'cupcake/state',
  HEARTBEAT: 'cupcake/sensors/heartbeat',
  OVERRIDE:  'cupcake/override',
};

export function useMqtt() {
  const [brokerConnected, setBrokerConnected] = useState(false);
  const [cupcakeState, setCupcakeState]       = useState(null);
  const [esp32Status, setEsp32Status]         = useState(null);
  const clientRef = useRef(null);

  useEffect(() => {
    const client = mqtt.connect(BROKER_WS_URL, {
      clientId: `cupcake-ui-${Date.now()}`,
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

  const sendOverride = useCallback((type, payload) => {
    clientRef.current?.publish(
      TOPICS.OVERRIDE,
      JSON.stringify({ type, ...payload })
    );
  }, []);

  return { brokerConnected, cupcakeState, esp32Status, sendOverride };
}
