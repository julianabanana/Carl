export default function Header({ connected, esp32Connected }) {
  return (
    <header className="flex items-center justify-between px-6 py-4 border-b border-slate-800 bg-slate-900">
      <div className="flex items-center gap-3">
        <div className="w-9 h-9 rounded-lg bg-violet-600 flex items-center justify-center text-white font-bold text-lg select-none">
          C
        </div>
        <div>
          <h1 className="text-lg font-semibold text-white leading-none">Cupcake</h1>
          <p className="text-xs text-slate-400 mt-0.5">Panel de Control</p>
        </div>
      </div>

      <div className="flex items-center gap-4">
        <Indicator label="Broker" active={connected} />
        <Indicator label="ESP32"  active={esp32Connected} />
      </div>
    </header>
  );
}

function Indicator({ label, active }) {
  return (
    <div className="flex items-center gap-1.5">
      <span
        className={`w-2 h-2 rounded-full ${active
          ? 'bg-emerald-400 shadow-[0_0_6px_#34d399]'
          : 'bg-slate-600'
        }`}
      />
      <span className={`text-xs font-medium ${active ? 'text-emerald-400' : 'text-slate-500'}`}>
        {label}
      </span>
    </div>
  );
}
