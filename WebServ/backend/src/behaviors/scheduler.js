import { publish } from '../mqtt/client.js';
import { TOPICS } from '../mqtt/topics.js';
import { STATES } from '../state/machine.js';

function randomMs(minS, maxS) {
  return (Math.random() * (maxS - minS) + minS) * 1000;
}

export function startScheduler(machine) {
  _scheduleBlink(machine);
  _startIdleWatcher(machine);
  _startSleepCheck(machine);
  console.log('[scheduler] Comportamientos autónomos activos');
}

// Parpadeo aleatorio cada 3-6 segundos
function _scheduleBlink(machine) {
  const tick = () => {
    if ([STATES.IDLE, STATES.CURIOUS, STATES.HAPPY].includes(machine.current)) {
      publish(TOPICS.CMD_ANIMATION, { id: 'pestanear' });
    }
    setTimeout(tick, randomMs(3, 6));
  };
  setTimeout(tick, randomMs(3, 6));
}

// Si lleva más de 10 segundos en idle, mira a los lados
function _startIdleWatcher(machine) {
  let investigarScheduled = false;

  setInterval(() => {
    if (machine.current !== STATES.IDLE) {
      investigarScheduled = false;
      return;
    }

    const idleSecs = (Date.now() - machine.since) / 1000;

    if (idleSecs > 10 && !investigarScheduled) {
      investigarScheduled = true;
      publish(TOPICS.CMD_ANIMATION, { id: 'investigar' });
      // Resetea para que vuelva a dispararse después de otros 10s de idle
      setTimeout(() => { investigarScheduled = false; }, 12000);
    }
  }, 2000);
}

// Cada minuto revisa si debe entrar/salir del modo nocturno
function _startSleepCheck(machine) {
  setInterval(() => {
    if (machine.isSleepHour() && machine.current === STATES.IDLE) {
      machine.transition(STATES.SLEEPY, 'horario nocturno');
    } else if (!machine.isSleepHour() && machine.current === STATES.SLEEPY) {
      machine.transition(STATES.IDLE, 'horario diurno');
    }
  }, 60 * 1000);
}
