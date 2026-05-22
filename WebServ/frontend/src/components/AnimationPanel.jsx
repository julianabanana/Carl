const ANIMATIONS = [
  { id: 'abrir',      label: 'Abrir',      description: 'Posición neutral',     icon: '👁️',  color: 'emerald' },
  { id: 'cerrar',     label: 'Cerrar',     description: 'Párpados cerrados',    icon: '😑',  color: 'slate'   },
  { id: 'pestanear',  label: 'Pestañear',  description: 'Parpadeo doble',       icon: '😉',  color: 'sky'     },
  { id: 'sorpresa',   label: 'Sorpresa',   description: 'Ojos y boca abiertos', icon: '😮',  color: 'amber'   },
  { id: 'investigar', label: 'Investigar', description: 'Mirar a los lados',    icon: '🔍',  color: 'violet'  },
];

const colorMap = {
  emerald: 'bg-emerald-500/10 border-emerald-500/30 text-emerald-300 hover:bg-emerald-500/20',
  slate:   'bg-slate-700/50  border-slate-600/50  text-slate-300   hover:bg-slate-700',
  sky:     'bg-sky-500/10    border-sky-500/30    text-sky-300     hover:bg-sky-500/20',
  amber:   'bg-amber-500/10  border-amber-500/30  text-amber-300   hover:bg-amber-500/20',
  violet:  'bg-violet-500/10 border-violet-500/30 text-violet-300  hover:bg-violet-500/20',
};

export default function AnimationPanel({ onAnimate, activeAnimation }) {
  return (
    <div className="bg-slate-900 rounded-xl border border-slate-800 p-5">
      <h2 className="text-sm font-semibold text-slate-300 uppercase tracking-wide mb-4">
        Animaciones
      </h2>
      <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-5 gap-3">
        {ANIMATIONS.map((anim) => (
          <button
            key={anim.id}
            onClick={() => onAnimate(anim.id)}
            className={`flex flex-col items-center gap-2 rounded-lg border p-3 transition-all duration-150 cursor-pointer
              ${colorMap[anim.color]}
              ${activeAnimation === anim.id ? 'ring-2 ring-offset-1 ring-offset-slate-900 ring-current scale-[1.02]' : ''}
            `}
          >
            <span className="text-2xl leading-none">{anim.icon}</span>
            <span className="text-xs font-semibold leading-none">{anim.label}</span>
            <span className="text-[10px] text-slate-500 text-center leading-tight">{anim.description}</span>
          </button>
        ))}
      </div>
    </div>
  );
}
