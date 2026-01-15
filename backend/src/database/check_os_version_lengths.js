const { Pool } = require("pg");
require("dotenv").config();

const pool = new Pool({ connectionString: process.env.DATABASE_URL });

async function check() {
  try {
    const res = await pool.query(
      `SELECT COUNT(*) AS total,
              SUM(CASE WHEN LENGTH(os_version) > 50 THEN 1 ELSE 0 END) AS too_long
       FROM devices`
    );

    console.log("Total devices:", res.rows[0].total);
    console.log("Devices with os_version length > 50:", res.rows[0].too_long);
  } catch (err) {
    console.error("Check failed:", err);
    process.exit(1);
  } finally {
    await pool.end();
  }
}

check();
