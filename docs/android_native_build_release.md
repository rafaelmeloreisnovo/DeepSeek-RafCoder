# Android Native Build + Release (JNI/NDK)

## Source of truth
- Android project root: `android/`
- Native core: `android/app/src/main/cpp/native-lib.cpp`
- ABI targets: `armeabi-v7a`, `arm64-v8a`
- CI workflow: `.github/workflows/android-native-ci.yml`

## Local build
Pré-requisito: Gradle disponível no PATH local.

```bash
./scripts/android_build_matrix.sh
```

## Signed release (local)
Set variables before running build:

```bash
export ANDROID_KEYSTORE_PATH=/absolute/path/release.keystore
export ANDROID_KEYSTORE_PASSWORD='***'
export ANDROID_KEY_ALIAS='***'
export ANDROID_KEY_PASSWORD='***'
./scripts/android_build_matrix.sh
```

## GitHub Actions secrets for signed release
- `ANDROID_KEYSTORE_BASE64`
- `ANDROID_KEYSTORE_PASSWORD`
- `ANDROID_KEY_ALIAS`
- `ANDROID_KEY_PASSWORD`

## Artifact map (CI)
- Debug APKs:
  - Build output: `android/app/build/outputs/apk/debug/*.apk`
  - Artifact name: `rafcoder-apk-debug`
- Unsigned release APKs:
  - Copied to: `android/artifacts/unsigned-release/*.apk`
  - Artifact name: `rafcoder-apk-release-unsigned`
- Signed release APKs (when signing secrets are configured):
  - Copied to: `android/artifacts/signed-release/*.apk`
  - Validation before upload: `apksigner verify --print-certs`
  - Artifact name: `rafcoder-apk-release-signed`

Without signing secrets CI still produces debug and unsigned release APKs.
