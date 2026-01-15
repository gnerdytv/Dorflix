const { Pool } = require("pg");
require("dotenv").config();

const pool = new Pool({ connectionString: process.env.DATABASE_URL });

async function migrate() {
  try {
    const res = await pool.query(
      "SELECT data_type FROM information_schema.columns WHERE table_name = 'devices' AND column_name = 'os_version'"
    );

    const currentType = res.rows[0] ? res.rows[0].data_type : null;
    console.log("Current os_version type:", currentType);

    if (currentType !== "text") {
      console.log("Altering os_version to TEXT...");
      await pool.query(
        "ALTER TABLE devices ALTER COLUMN os_version TYPE TEXT USING os_version::text"
      );
      console.log("Migration completed: os_version is now TEXT");
    } else {
      console.log("No migration needed: os_version is already TEXT");
    }
  } catch (err) {
    console.error("Migration failed:", err);
    process.exit(1);
  } finally {
    await pool.end();
  }
}

migrate();
