import { publish } from '../mqtt/client.js';
import { TOPICS } from '../mqtt/topics.js';

export const STATES = {
  IDLE:     'idle',
  CURIOUS:  'curious',
  SLEEPY:   'sleepy',
  HAPPY:    'happy',
  STARTLED: 'startled',
};

// Animación que se dispara al entrar a cada estado
const ENTRY_ANIMATIONS = {
  [STATES.CURIOUS]:  'investigar',
  [STATES.HAPPY]:    'abrir',
  [STATES.STARTLED]: 'sorpresa',
  [STATES.SLEEPY]:   null,
  [STATES.IDLE]:     null,
};

// Cuántos ms dura el estado antes de volver a idle (null = no vuelve solo)
const AUTO_RETURN_MS = {
  [STATES.STARTLED]: 2000,
  [STATES.HAPPY]:    3000,
};

export class CupcakeMachine {
  constructor() {
    this.current = STATES.IDLE;
    this.since = Date.now();
    this._returnTimer = null;
  }

  transition(newState, reason = '') {
    if (this.current === newState) return;
    console.log(`[state] ${this.current} → ${newState}  (motivo: ${reason})`);

    clearTimeout(this._returnTimer);
    this._returnTimer = null;

    this.current = newState;
    this.since = Date.now();

    this._onEnter(newState);
    this._publishState();
  }

  handleSensor(topic, data) {
    if (!topic.includes('distance')) return;

    const dist = data.value;

    if (dist < 30 && this.current !== STATES.STARTLED) {
      this.transition(STATES.STARTLED, `distancia crítica: ${dist}cm`);
    } else if (dist < 80 && this.current === STATES.IDLE) {
      this.transition(STATES.CURIOUS, `proximidad detectada: ${dist}cm`);
    } else if (dist > 100 && this.current === STATES.CURIOUS) {
      this.transition(STATES.IDLE, `sin proximidad: ${dist}cm`);
    }
  }

  handleOverride(data) {
    switch (data.type) {
      case 'state': {
        const target = STATES[data.state?.toUpperCase()];
        if (target) this.transition(target, 'override manual');
        break;
      }
      case 'animation':
        publish(TOPICS.CMD_ANIMATION, { id: data.id });
        break;
      case 'servo':
        publish(TOPICS.CMD_SERVO, { channel: data.channel, angle: data.angle });
        break;
      default:
        console.warn('[state] Override desconocido:', data.type);
    }
  }

  isSleepHour() {
    const hour = new Date().getHours();
    const start = parseInt(process.env.SLEEP_HOUR_START ?? '22');
    const end   = parseInt(process.env.SLEEP_HOUR_END   ?? '8');
    return hour >= start || hour < end;
  }

  getStatus() {
    return {
      current:   this.current,
      since:     this.since,
      uptime_ms: Date.now() - this.since,
      timestamp: Date.now(),
    };
  }

  _onEnter(state) {
    const anim = ENTRY_ANIMATIONS[state];
    if (anim) publish(TOPICS.CMD_ANIMATION, { id: anim });

    const returnMs = AUTO_RETURN_MS[state];
    if (returnMs) {
      this._returnTimer = setTimeout(
        () => this.transition(STATES.IDLE, `timeout de ${state}`),
        returnMs
      );
    }
  }

  _publishState() {
    publish(TOPICS.STATE, this.getStatus());
  }
}
