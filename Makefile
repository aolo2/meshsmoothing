all: debug

debug:
	@echo "[INFO] DEBUG build"
	@+make -f build/debug.mk all --no-print-directory

release:
	@echo "[INFO] RELEASE build"
	@+make -f build/release.mk all --no-print-directory

profile:
	@echo "[INFO] PROFILE build (tracy, full)"
	@+make -f build/profile.mk all --no-print-directory

profile_fast:
	@echo "[INFO] PROFILE build (tracy, fast)"
	@+make -f build/profile.mk fast --no-print-directory

analyze:
	@echo "[INFO] ANALYZE (gcc-10)"
	@+make -f build/analyze.mk --no-print-directory

.PHONY:
	all debug release profile profile_fast analyze
