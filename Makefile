all: debug

debug:
	@echo "[INFO] DEBUG build"
	@+make -f build/debug.mk all --no-print-directory

release:
	@echo "[INFO] RELEASE build"
	@+make -f build/release.mk all --no-print-directory

profile:
	@echo "[INFO] PROFILE build (tracy)"
	@+make -f build/profile.mk all --no-print-directory

.PHONY:
	all debug release profile
