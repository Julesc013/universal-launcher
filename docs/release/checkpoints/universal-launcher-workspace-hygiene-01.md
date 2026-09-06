# Universal Launcher workspace hygiene checkpoint

Date: 2026-08-31  
WorkUnit: `UNIVERSAL-LAUNCHER-WORKSPACE-HYGIENE-01`

The repository now carries the same marker-owned, target-aware hygiene core as
FacMan. Its policy is provider-specific only where the worktree ceiling is
stricter: Universal Launcher normally permits one secondary worktree.

Before this WorkUnit, stale Git administrative entries were pruned without
deleting filesystem content. Five exact legacy worktrees were then inspected
individually, found clean, and removed with plain `git worktree remove`:

- detached `e6de83ad1e1a2c646d31eb2ca68aa5cddb323b4a`, reachable from `origin/dev`;
- detached `09f0639ab6529fba2f2aa22e9bf68e5eebed0553`, reachable from `origin/dev`;
- detached `1cafe4054297cc11e02458b83d230db0cd064471`, reachable from `origin/dev`;
- `task/ulk-session-last-run-promotion-01` at `74bc7018b5ba0be3e21d03ce71a0a2aedd7f5bc9`, exact head merged by PR #13;
- `task/ulk-session-last-run-subset-01` at `6a6e49336d66fb95ce6efce90d480d60f93e66ba`, exact head merged by PR #12.

Neither merged branch had an open dependent PR. Their local and remote refs
were deleted separately after worktree removal. The control checkout remained
clean and synchronized on `dev`. The new external WorkUnit worktree was then
created under the canonical marker-owned store.

The repository merge controls now allow merge commits only, automatically
delete merged branches, and leave auto-merge disabled.

Nine additional remote task refs whose tips were already reachable from
`origin/dev` and had no open head or dependent PR were deleted, along with four
matching merged local refs. Open PR #18 and its local/remote branch were
retained. The only unmerged local-only legacy branch, `task/ulaunch-min-01` at
`3b64f1ea9bd1a6181b9c6863985c2c099804c692`, was preserved before ref deletion
in the verified complete-history bundle:

```text
D:\Projects\Factorio\.backups\universal-launcher\ulk-ulaunch-min-01-2026-08-31.bundle
SHA-256 1137AE476824D05552C637B52C15FC5F0B8D3BDC1BDD139DA4B4AD646B1988D3
```
