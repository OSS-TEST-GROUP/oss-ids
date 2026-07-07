@Library('sbom-library') _

pipeline {
  agent {
    docker {
      image 'dds-ids-builder:latest'
      args '-u root'
      label ''
      customWorkspace "/workspace/dds-ids"
    }
  }

  options {
    buildDiscarder(logRotator(numToKeepStr: '10', artifactNumToKeepStr: '3'))
    timestamps()
    disableConcurrentBuilds()
  }

  parameters {
    booleanParam(name: 'CLEAN_BUILD', defaultValue: false, description: 'Remove previous build outputs before running')
    booleanParam(name: 'GENERATE_SBOM', defaultValue: true, description: 'Generate a CycloneDX SBOM and upload it to SBOM Manager')
  }

  environment {
    CONAN_REMOTE_NAME = 'oss'
    CONAN_REMOTE_URL  = 'https://gitlab.com/api/v4/projects/83664124/packages/conan'
    TEST_RESULTS_DIR  = 'build/Release/test-results'
    SBOM_PROJECT_ID   = 'd719306b-cd1d-4e47-81ce-6caf7e6e0afb'
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
        script {
          env.GIT_COMMIT_SHORT = sh(script: 'git rev-parse --short HEAD', returnStdout: true).trim()
          env.GIT_VERSION = sh(script: 'git describe --tags --always 2>/dev/null || git rev-parse --short HEAD', returnStdout: true).trim()
          def branch = env.BRANCH_NAME ?: env.GIT_BRANCH ?: 'detached'
          env.SBOM_VERSION = "${branch}-${env.GIT_COMMIT_SHORT}"
          echo "branch: ${branch} | commit: ${env.GIT_COMMIT_SHORT} | version: ${env.GIT_VERSION}"
          sh 'git fetch --tags --force || true'
        }
      }
    }

    stage('Prepare') {
      steps {
        sh '''
          set -e
          cmake --version
          conan --version
          gcc --version
          g++ --version
          if [ "${CLEAN_BUILD}" = "true" ]; then
            rm -rf build artifacts
          fi
          conan remote add "${CONAN_REMOTE_NAME}" "${CONAN_REMOTE_URL}" --force
        '''
      }
    }

    stage('Build') {
      steps {
        sh '''
          set -e
          conan profile detect --force
          conan install . --build=missing \
            -s build_type=Release \
            -s compiler.cppstd=gnu17
          conan lock create . \
            -s build_type=Release \
            -s compiler.cppstd=gnu17
          cmake -S . -B build/Release \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
            -DBUILD_TESTING=ON
          cmake --build build/Release -j "$(nproc)"
        '''
      }
    }

    stage('Test') {
      steps {
        sh '''
          set -e
          mkdir -p "${TEST_RESULTS_DIR}"
          ctest --test-dir build/Release \
            --output-on-failure \
            --output-junit "${TEST_RESULTS_DIR}/ctest-results.xml"
        '''
      }
      post {
        always {
          junit allowEmptyResults: true, testResults: 'build/Release/test-results/**/*.xml'
        }
      }
    }

    stage('Artifacts') {
      steps {
        sh '''
          set -e
          VERSION="${GIT_VERSION}"
          STAGE="artifacts/dds-ids_${VERSION}"

          rm -rf artifacts
          mkdir -p "${STAGE}/bin" "${STAGE}/etc"

          for BIN in SecurityManager SecurityClient; do
            if [ -f "build/Release/bin/${BIN}" ]; then
              cp "build/Release/bin/${BIN}" "${STAGE}/bin/"
            fi
          done
          cp -r policy_rule/. "${STAGE}/etc/"

          tar -czf "artifacts/dds-ids_${VERSION}.tar.gz" -C artifacts "dds-ids_${VERSION}"
          rm -rf "${STAGE}"

          printf "BRANCH=%s\nCOMMIT=%s\nBUILD_TIME=%s\n" \
            "${BRANCH_NAME:-detached}" "${GIT_COMMIT_SHORT}" "$(date '+%Y-%m-%d %H:%M:%S')" \
            > artifacts/build-info.txt
          ls -lh artifacts/
        '''
        archiveArtifacts artifacts: 'artifacts/*.tar.gz, artifacts/build-info.txt', fingerprint: true, allowEmptyArchive: true
      }
    }

    stage('SBOM') {
      when {
        expression { params.GENERATE_SBOM }
      }
      steps {
        catchError(buildResult: 'SUCCESS', stageResult: 'FAILURE') {
          script {
            sbomPython.generateAndUpload(
              projectId: env.SBOM_PROJECT_ID,
              projectName: 'dds-ids',
              version: env.SBOM_VERSION)
          }
        }
      }
      post {
        failure { echo 'SBOM generation or upload failed.' }
      }
    }
  }

  post {
    success  { echo "build success: ${env.BRANCH_NAME ?: 'detached'} @ ${env.GIT_COMMIT_SHORT}" }
    failure  { echo "build failed: ${env.BRANCH_NAME ?: 'detached'} @ ${env.GIT_COMMIT_SHORT}" }
    unstable { echo "build unstable: ${env.BRANCH_NAME ?: 'detached'} @ ${env.GIT_COMMIT_SHORT}" }
  }
}
