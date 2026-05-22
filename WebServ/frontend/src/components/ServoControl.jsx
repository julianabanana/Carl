export default function ServoControl({ label, channel, value, min = 0, max = 180, onChange }) {
  const percentage = ((value - min) / (max - min)) * 100;

  return (
    <div className="bg-slate-800/60 rounded-lg p-4 border border-slate-700/50">
      <div className="flex justify-between items-center mb-3">
        <div>
          <p className="text-sm font-medium text-slate-200">{label}</p>
          <p className="text-xs text-slate-500">Canal {channel}</p>
        </div>
        <span className="text-sm font-mono font-bold text-violet-400 bg-violet-500/10 px-2 py-1 rounded">
          {value}°
        </span>
      </div>

      <input
        type="range"
        min={min}
        max={max}
        value={value}
        onChange={(e) => onChange(channel, Number(e.target.value))}
        className="w-full h-2 rounded-full appearance-none cursor-pointer bg-slate-700
                   [&::-webkit-slider-thumb]:appearance-none
                   [&::-webkit-slider-thumb]:w-4
                   [&::-webkit-slider-thumb]:h-4
                   [&::-webkit-slider-thumb]:rounded-full
                   [&::-webkit-slider-thumb]:bg-violet-500
                   [&::-webkit-slider-thumb]:cursor-pointer
                   [&::-webkit-slider-thumb]:shadow-[0_0_6px_#7c3aed]"
        style={{
          background: `linear-gradient(to right, #7c3aed ${percentage}%, #334155 ${percentage}%)`,
        }}
      />
      <div className="flex justify-between mt-1">
        <span className="text-xs text-slate-600">{min}°</span>
        <span className="text-xs text-slate-600">{max}°</span>
      </div>
    </div>
  );
}
