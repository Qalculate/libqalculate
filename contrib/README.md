Contributed tools for libqalcalulate
====================================

- `qalc_refresh_rates.service` and `qalc_refresh_rates.timer` …
  add both files to `~/.config/systemd/user/` and enable with
  `systemctl --user enable --now qalc_refresh_rates.timer` and
  systemd will refresh daily (in 6:00 AM) exchange rates for
  qalc.
