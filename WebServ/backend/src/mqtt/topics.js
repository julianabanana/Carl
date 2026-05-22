// Tópicos MQTT del sistema Cupcake
//
// Flujo general:
//   ESP32 publica en sensors/   → Backend lee y toma decisiones
//   Backend publica en commands/ → ESP32 ejecuta
//   Backend publica en state     → Frontend muestra
//   Frontend publica en override → Backend procesa override manual

export const TOPICS = {
  // ESP32 → todos
  SENSOR_HEARTBEAT: 'cupcake/sensors/heartbeat',   // {uptime, freeHeap}
  SENSOR_DISTANCE:  'cupcake/sensors/distance',    // {value: cm}

  // Backend → ESP32
  CMD_SERVO:        'cupcake/commands/servo',       // {channel, angle}
  CMD_ANIMATION:    'cupcake/commands/animation',   // {id}

  // Backend → Frontend
  STATE:            'cupcake/state',                // {current, since, timestamp}

  // Frontend → Backend
  OVERRIDE:         'cupcake/override',             // {type: "state"|"animation"|"servo", ...}
};

// Wildcard para suscribirse a todos los sensores
export const SENSOR_WILDCARD = 'cupcake/sensors/#';
