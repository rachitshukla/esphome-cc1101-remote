# Repository instructions

## Credentials and keys

- Keep real Wi-Fi credentials, ESPHome API encryption keys, OTA passwords, access tokens, and private keys out of Git. Use ESPHome `!secret` references and keep their values in the ignored local `secrets.yaml`. Supply GitHub credentials through the credential manager.
- Use placeholders or synthetic test values in documentation, fixtures, and build checks. Treat generated source, compiled firmware, flash backups, captures, and logs as local artifacts because they can contain expanded secrets. The published RF button codes are intentional protocol examples.
- Before each commit, stage files by name. Run `git diff --cached --name-status` and privately inspect the full staged changes, including new files. Check unfamiliar configuration values, binary files, and generated output before committing. Never paste suspected secrets into chat, review comments, or commit messages.
- Use `git check-ignore -v -- secrets.yaml` to verify the ignore rule and `git ls-files -- secrets.yaml` to check the index. The second command should print nothing. Ignore rules do not protect files already tracked. Never force-add local credential files.
- If a credential is staged, unstage it and replace the tracked value with a reference. If one has already been committed, stop the push and tell the owner privately which credential and path were affected, omitting the value. Coordinate rotation or revocation and any history cleanup with the owner.
