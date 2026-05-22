const STATE_CONFIG = {
  idle:     { label: 'Idle',      icon: '😐', color: 'slate',   description: 'En reposo'              },
  curious:  { label: 'Curiosa',   icon: '🤔', color: 'sky',     description: 'Alguien se acerca'      },
  sleepy:   { label: 'Dormida',   icon: '😴', color: 'violet',  description: 'Modo nocturno'          },
  happy:    { label: 'Feliz',     icon: '😊', color: 'emerald', description: 'Interacción positiva'   },
  startled: { label: 'Asustada',  icon: '😮', color: 'amber',   description: 'Movimiento súbito'      },
};

const COLOR_CLASSES = {
  slate:   'border-slate-600/50  bg-slate-700/30  text-slate-300',
  sky:     'border-sky-500/40    bg-sky-500/10    text-sky-300',
  violet:  'border-violet-500/40 bg-violet-500/10 text-violet-300',
  emerald: 'border-emerald-500/40 bg-emerald-500/10 text-emerald-300',
  amber:   'border-amber-500/40  bg-amber-500/10  text-amber-300',
};

const OVERRIDE_STATES = Object.entries(STATE_CONFIG);

function elapsed(sinceMs) {
  const s = Math.floor((Date.now() - sinceMs) / 1000);
  if (s < 60) return `${s}s`;
  return `${Math.floor(s / 60)}m ${s % 60}s`;
}

export default function StateDisplay({ cupcakeState, onOverrideState }) {
  const state  = cupcakeState?.current ?? 'idle';
  const config = STATE_CONFIG[state] ?? STATE_CONFIG.idle;

  return (
    <div className="bg-slate-900 rounded-xl border border-slate-800 p-5 space-y-4">
      <h2 className="text-sm font-semibold text-slate-300 uppercase tracking-wide">
        Estado de Cupcake
      </h2>

      {/* Estado actual */}
      <div className={`flex items-center gap-4 rounded-lg border p-4 ${COLOR_CLASSES[config.color]}`}>
        <span className="text-4xl leading-none">{config.icon}</span>
        <div className="flex-1">
          <p className="text-lg font-bold leading-none">{config.label}</p>
          <p className="text-xs opacity-70 mt-1">{config.description}</p>
        </div>
        {cupcakeState && (
          <p className="text-xs font-mono opacity-50">{elapsed(cupcakeState.since)}</p>
        )}
        {!cupcakeState && (
          <p className="text-xs text-slate-500 italic">esperando datos...</p>
        )}
      </div>

      {/* Botones de override */}
      <div>
        <p className="text-xs text-slate-500 mb-2">Override manual</p>
        <div className="flex flex-wrap gap-2">
          {OVERRIDE_STATES.map(([id, cfg]) => (
            <button
              key={id}
              onClick={() => onOverrideState(id)}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg border text-xs font-medium
                transition-all cursor-pointer
                ${state === id
                  ? `${COLOR_CLASSES[cfg.color]} ring-1 ring-current`
                  : 'border-slate-700 bg-slate-800 text-slate-400 hover:border-slate-600 hover:text-slate-200'
                }`}
            >
              <span>{cfg.icon}</span>
              {cfg.label}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}
