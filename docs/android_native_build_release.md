# Android Native Build + Release (JNI/NDK)

## Source of truth
- Android project root: `android/`
- Native core: `android/app/src/main/cpp/native-lib.cpp`
- ABI matrix oficial (Gradle `ndk.abiFilters` + CMake): `armeabi-v7a`, `arm64-v8a`
- `x86_64` não é empacotado na trilha oficial (release/CI)
- CI workflow: `.github/workflows/android-native-ci.yml`
- Gradle execution path (official): `android/gradlew`
- Gradle wrapper version: `8.14.3`
- Wrapper JAR bootstrap: `scripts/ensure_gradle_wrapper_jar.sh` (não versiona binário no repositório)

## Local build
Pré-requisito: Java (JDK 17+) disponível no PATH local.

```bash
./scripts/bootstrap_gradle_wrapper.sh
./scripts/android_build_matrix.sh
```

Esse script chama `scripts/ensure_gradle_wrapper_jar.sh` e depois `android/gradlew` automaticamente.

## Signed release (local)
Set variables before running build:

```bash
export ANDROID_KEYSTORE_PATH=/absolute/path/release.keystore
export ANDROID_KEYSTORE_PASSWORD='***'
export ANDROID_KEY_ALIAS='***'
export ANDROID_KEY_PASSWORD='***'
./scripts/android_build_matrix.sh
```

Esse script chama `scripts/ensure_gradle_wrapper_jar.sh` e depois `android/gradlew` automaticamente.

## GitHub Actions secrets for signed release
- `ANDROID_KEYSTORE_BASE64`
- `ANDROID_KEYSTORE_PASSWORD`
- `ANDROID_KEY_ALIAS`
- `ANDROID_KEY_PASSWORD`

Without these secrets CI still produces unsigned debug/release APKs.

## Artifact map (CI)
### `android/artifacts/debug`
- `rafcoder-apk-debug-armeabi-v7a` -> `android/artifacts/debug/rafcoder-armeabi-v7a-debug.apk`
- `rafcoder-apk-debug-arm64-v8a` -> `android/artifacts/debug/rafcoder-arm64-v8a-debug.apk`

### `android/artifacts/unsigned-release`
- `rafcoder-apk-release-unsigned-armeabi-v7a` -> `android/artifacts/unsigned-release/rafcoder-armeabi-v7a-release-unsigned.apk`
- `rafcoder-apk-release-unsigned-arm64-v8a` -> `android/artifacts/unsigned-release/rafcoder-arm64-v8a-release-unsigned.apk`

### `android/artifacts/signed-release` (quando signing secrets existirem)
- `rafcoder-apk-release-signed-armeabi-v7a` -> `android/artifacts/signed-release/rafcoder-armeabi-v7a-release-signed.apk`
- `rafcoder-apk-release-signed-arm64-v8a` -> `android/artifacts/signed-release/rafcoder-arm64-v8a-release-signed.apk`

## Wrapper/Gradle version policy
- Official entrypoint for Android builds: `./android/gradlew` (local + CI) and `android/gradlew.bat` on Windows.
- Wrapper JAR bootstrap: `./scripts/bootstrap_gradle_wrapper.sh` (fetches `android/gradle/wrapper/gradle-wrapper.jar` em runtime no CI/local, não versionar binário no repositório).
- Gradle version is pinned to `8.14.3` in `android/gradle/wrapper/gradle-wrapper.properties` and CI enforces this same version via `GRADLE_VERSION=8.14.3`.
