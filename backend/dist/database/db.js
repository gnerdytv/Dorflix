"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.query = exports.connectDatabase = exports.testConnection = void 0;
const pg_1 = require("pg");
const dotenv_1 = __importDefault(require("dotenv"));
dotenv_1.default.config();
// Database connection configuration
const pool = new pg_1.Pool({
    connectionString: process.env.DATABASE_URL ||
        `postgresql://${process.env.DB_USER}:${process.env.DB_PASSWORD}@${process.env.DB_HOST}:${process.env.DB_PORT}/${process.env.DB_NAME}`,
    ssl: process.env.NODE_ENV === "production"
        ? { rejectUnauthorized: false }
        : false,
});
// Test database connection
const testConnection = async () => {
    try {
        const client = await pool.connect();
        console.log("✅ Database connected successfully");
        client.release();
        return true;
    }
    catch (error) {
        console.error("❌ Database connection failed:", error);
        return false;
    }
};
exports.testConnection = testConnection;
// Connect to database
const connectDatabase = async () => {
    try {
        await (0, exports.testConnection)();
        console.log("Database connection established");
    }
    catch (error) {
        console.error("Failed to connect to database:", error);
        throw error;
    }
};
exports.connectDatabase = connectDatabase;
// Database query helper
const query = async (text, params) => {
    const start = Date.now();
    try {
        const res = await pool.query(text, params);
        const duration = Date.now() - start;
        console.log("✅ Executed query", { text, duration, rows: res.rowCount });
        return res;
    }
    catch (error) {
        console.error("❌ Query error:", { text, params, error });
        throw error;
    }
};
exports.query = query;
exports.default = pool;
//# sourceMappingURL=db.js.map