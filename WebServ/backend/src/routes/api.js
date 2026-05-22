import { Router } from 'express';
import { publish } from '../mqtt/client.js';
import { TOPICS } from '../mqtt/topics.js';
import { STATES } from '../state/machine.js';

export function createRouter(machine) {
  const router = Router();

  // Estado actual de Cupcake
  router.get('/state', (_req, res) => {
    res.json(machine.getStatus());
  });

  // Override de estado
  router.post('/override/state', (req, res) => {
    const { state } = req.body;
    const target = STATES[state?.toUpperCase()];
    if (!target) {
      return res.status(400).json({ error: `Estado inválido: ${state}. Válidos: ${Object.values(STATES).join(', ')}` });
    }
    machine.transition(target, 'override via API');
    res.json({ ok: true, state: machine.getStatus() });
  });

  // Override de animación
  router.post('/override/animation', (req, res) => {
    const { id } = req.body;
    if (!id) return res.status(400).json({ error: 'Falta el campo id' });
    publish(TOPICS.CMD_ANIMATION, { id });
    res.json({ ok: true, animation: id });
  });

  // Override de servo individual
  router.post('/override/servo', (req, res) => {
    const { channel, angle } = req.body;
    if (channel === undefined || angle === undefined) {
      return res.status(400).json({ error: 'Faltan channel y/o angle' });
    }
    publish(TOPICS.CMD_SERVO, { channel: Number(channel), angle: Number(angle) });
    res.json({ ok: true, channel, angle });
  });

  return router;
}
