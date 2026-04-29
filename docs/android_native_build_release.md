# Android Native Build + Release (JNI/NDK)

## Source of truth
- Android project root: `android/`
- Native core: `android/app/src/main/cpp/native-lib.cpp`
- ABI targets: `armeabi-v7a`, `arm64-v8a`
- CI workflow: `.github/workflows/android-native-ci.yml`

## Local build
Pré-requisito: usar o Gradle Wrapper oficial em `android/gradlew` e inicializar o bootstrap do wrapper jar.

```bash
./scripts/bootstrap_gradle_wrapper.sh
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

Without these secrets CI still produces unsigned debug/release APKs.

## Wrapper/Gradle version policy
- Official entrypoint for Android builds: `./android/gradlew` (local + CI).
- Wrapper JAR bootstrap: `./scripts/bootstrap_gradle_wrapper.sh` (fetches `android/gradle/wrapper/gradle-wrapper.jar` locally/CI, not committed).
- Wrapper distribution pinned in `android/gradle/wrapper/gradle-wrapper.properties` and must stay aligned with CI expectations.
