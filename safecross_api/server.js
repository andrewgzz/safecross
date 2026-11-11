const express = require('express');
const mysql = require('mysql2');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(express.json());

const db = mysql.createConnection({
  host: 'localhost',
  user: 'safecross_user',
  password: 'tu_contraseña_segura', // Tu contraseña configurada
  database: 'safe_cross_db',
  connectTimeout: 5000
});

db.connect(err => {
  if (err) {
    console.error('❌ Error conectando a MySQL:', err.message);
    return;
  }
  console.log('✅ Conectado a la base de datos MySQL');
});

app.post('/api/mediciones', (req, res) => {
  console.log('📩 Petición POST recibida en /api/mediciones:', req.body);
  
  const { id_hw, valor_medido } = req.body;
  
  if (id_hw === undefined || valor_medido === undefined) {
    return res.status(400).json({ error: 'Faltan datos (id_hw, valor_medido)' });
  }

  const sql = 'INSERT INTO MEDICION_TELEMETRIA (id_hw, valor_medido) VALUES (?, ?)';
  
  db.query(sql, [id_hw, valor_medido], (err, result) => {
    if (err) {
      console.error('❌ Error en INSERT SQL:', err.message);
      return res.status(500).json({ error: 'Error interno en BD', detalle: err.message });
    }
    
    console.log(`📡 Medición guardada con éxito (ID Inserto: ${result.insertId})`);
    return res.status(201).json({ message: 'OK', id: result.insertId });
  });
});

app.get('/api/ping', (req, res) => {
  console.log('🔔 Ping recibido');
  res.json({ status: 'ok', mensaje: 'Servidor respondiendo' });
});

const PORT = 3000;
app.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Servidor API REST activo en http://0.0.0.0:${PORT}`);
});
