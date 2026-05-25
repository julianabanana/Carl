import { useState } from 'react';
import { useMqtt } from './hooks/useMqtt';
import Header from './components/Header';
import StatusCard from './components/StatusCard';
import ServoControl from './components/ServoControl';
import AnimationPanel from './components/AnimationPanel';
import StateDisplay from './components/StateDisplay';

const INITIAL_SERVOS = [
  { channel: 0, label: 'Párpados Superiores', min: 10,  max: 90,  value: 90 },
  { channel: 1, label: 'Párpados Inferiores', min: 25,  max: 102, value: 50 },
  { channel: 2, label: 'Ojo Izq. Horizontal', min: 0,   max: 180, value: 90 },
  { channel: 3, label: 'Ojo Izq. Vertical',   min: 0,   max: 180, value: 90 },
  { channel: 4, label: 'Ojo Der. Horizontal', min: 0,   max: 180, value: 90 },
  { channel: 5, label: 'Ojo Der. Vertical',   min: 0,   max: 180, value: 90 },
  { channel: 6, label: 'Boca (Mandíbula)',    min: 0,   max: 180, value: 90 },
];

export default function App() {
  const { brokerConnected, cupcakeState, esp32Status, sendAnimation, sendServo, sendStateOverride } = useMqtt();
  const [servos, setServos] = useState(INITIAL_SERVOS);
  const [activeAnimation, setActiveAnimation] = useState(null);

  function handleServoChange(channel, value) {
    setServos((prev) =>
      prev.map((s) => (s.channel === channel ? { ...s, value } : s))
    );
    sendServo(channel, value);
  }

  function handleAnimate(animId) {
    setActiveAnimation(animId);
    sendAnimation(animId);
    setTimeout(() => setActiveAnimation(null), 1500);
  }

  function handleOverrideState(state) {
    sendStateOverride(state);
  }

  const esp32Connected = Boolean(esp32Status);
  const uptime   = esp32Status ? formatUptime(esp32Status.uptime)  : '—';
  const freeHeap = esp32Status ? `${Math.round(esp32Status.freeHeap / 1024)} KB` : '—';

  return (
    <div className="min-h-screen bg-[#0f1117] flex flex-col">
      <Header connected={brokerConnected} esp32Connected={esp32Connected} />

      <main className="flex-1 p-6 max-w-6xl mx-auto w-full space-y-6">

        {/* Tarjetas de estado */}
        <section>
          <h2 className="text-xs font-semibold text-slate-500 uppercase tracking-wide mb-3">
            Estado del sistema
          </h2>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
            <StatusCard label="Broker MQTT" value={brokerConnected ? 'Online' : 'Offline'} color={brokerConnected ? 'emerald' : 'amber'} />
            <StatusCard label="ESP32"       value={esp32Connected  ? 'Online' : 'Offline'} color={esp32Connected  ? 'emerald' : 'amber'} />
            <StatusCard label="Uptime"      value={uptime}                                  color="sky"    />
            <StatusCard label="Memoria"     value={freeHeap}                                color="violet" />
          </div>
        </section>

        {/* Estado de Cupcake (mascota) */}
        <StateDisplay
          cupcakeState={cupcakeState}
          onOverrideState={handleOverrideState}
        />

        {/* Panel de animaciones */}
        <AnimationPanel onAnimate={handleAnimate} activeAnimation={activeAnimation} />

        {/* Control de servos */}
        <section className="bg-slate-900 rounded-xl border border-slate-800 p-5">
          <h2 className="text-sm font-semibold text-slate-300 uppercase tracking-wide mb-4">
            Control de Servos
          </h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
            {servos.map((servo) => (
              <ServoControl
                key={servo.channel}
                {...servo}
                onChange={handleServoChange}
              />
            ))}
          </div>
        </section>

      </main>

      <footer className="text-center py-4 text-xs text-slate-600 border-t border-slate-800">
        Cupcake Animatrónico — Panel de Control v0.1
      </footer>
    </div>
  );
}

function formatUptime(ms) {
  if (!ms) return '—';
  const s = Math.floor(ms / 1000);
  const h = Math.floor(s / 3600).toString().padStart(2, '0');
  const m = Math.floor((s % 3600) / 60).toString().padStart(2, '0');
  const sec = (s % 60).toString().padStart(2, '0');
  return `${h}:${m}:${sec}`;
}
