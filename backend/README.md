# Backend README (Dorflix)

This file contains backend-specific developer notes and quick commands for database maintenance (migrations and checks).

## Quick commands

- Start dev server: `npm run dev` (uses `tsx` to watch `src/index.ts`).
- Run existing migrations: `npm run migrate` (runs compiled `dist/database/migrate.js`).

### New helpful scripts

- Check how many devices have long `os_version` values:
  - `npm run check:os-version-lengths` (runs `src/database/check_os_version_lengths.js`).
- Migrate `os_version` to `TEXT` (idempotent):
  - `npm run migrate:os-version-to-text` (runs `src/database/migrate_change_os_version_to_text.js`).

## Migration checklist (recommended)

1. Backup your database (dump or snapshot).
2. Run `npm run check:os-version-lengths` to see how many rows would have been truncated by the old VARCHAR(50).
3. Run `npm run migrate:os-version-to-text` to alter the `os_version` column to `TEXT` (idempotent).
4. Verify the change: connect to the DB and run:
   - `SELECT column_name, data_type FROM information_schema.columns WHERE table_name = 'devices' AND column_name = 'os_version';`
5. Monitor logs for surprising values and ensure downstream code handles larger `os_version` strings.

## Notes

- This change was made because some clients (especially web user-agents) may provide long strings that previously caused `value too long for type character varying(50)` errors.
- We keep a short-term defensive truncation in `backend/src/services/deviceService.ts` as an extra safety net and log warnings when truncation occurs.

If you'd like, I can open a PR that includes this README and references the migration scripts in the main project docs as well.
