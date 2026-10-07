#!/bin/bash
set -e # 에러 발생 시 즉시 중단 (안전 가드)

# 1. 외부 사양서 파일 정의 및 존재 여부 검증
CONFIG_FILE="./msg_list.txt"
if [ ! -f "${CONFIG_FILE}" ]; then
    echo "❌ 에러: 설정 파일 '${CONFIG_FILE}'을 찾을 수 없습니다."
    exit 1
fi

# 2. 스크립트를 호출한 현재 호스트 사용자의 UID와 GID 동적 캡처
USER_UID=$(id -u)
USER_GID=$(id -g)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TARGET_DIR="${SCRIPT_DIR}/target_idl"
rm -rf "${TARGET_DIR}" # 이전 찌꺼기 완벽 초기화
mkdir -p "${TARGET_DIR}/std_msgs/msg"
mkdir -p "${TARGET_DIR}/builtin_interfaces/msg"

DOCKER_IMAGE="ros:humble-ros-base"
TMP_BUILD_DIR="${SCRIPT_DIR}/.tmp_ws_src"
PKG_LIST_FILE="${TMP_BUILD_DIR}/packages.txt"

create_ros_interface_package() {
    local pkg_dir="$1"
    local pkg_name="$2"

    cat > "${pkg_dir}/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.15)
project(${pkg_name})
find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)

file(GLOB MSG_FILES RELATIVE "\${CMAKE_CURRENT_SOURCE_DIR}" "msg/*.msg")

rosidl_generate_interfaces(\${PROJECT_NAME} \${MSG_FILES}
  DEPENDENCIES std_msgs builtin_interfaces)
ament_package()
EOF

    cat > "${pkg_dir}/package.xml" <<EOF
<?xml version="1.0"?>
<package format="3">
  <name>${pkg_name}</name>
  <version>0.0.0</version>
  <description>Temporary generator wrapper</description>
  <maintainer email="pdev@todo.todo">pdev</maintainer>
  <license>Apache-2.0</license>
  <buildtool_depend>ament_cmake</buildtool_depend>
  <buildtool_depend>rosidl_default_generators</buildtool_depend>
  <depend>std_msgs</depend>
  <depend>builtin_interfaces</depend>
  <member_of_group>rosidl_interface_packages</member_of_group>
  <export><build_type>ament_cmake</build_type></export>
</package>
EOF
}

echo "▶ [1/3] ROS 2 도커 샌드박스 가동 및 기본 의존성 IDL 추출 중..."

# 3. 공식 표준 의존성 IDL 추출 (호스트의 target_idl 경로에 직접 복사)
docker run --rm -i \
  -v "${SCRIPT_DIR}:/workspace" \
  ${DOCKER_IMAGE} \
  bash <<'EOF'
set -e
cp /opt/ros/humble/share/std_msgs/msg/Header.idl /workspace/target_idl/std_msgs/msg/
cp /opt/ros/humble/share/builtin_interfaces/msg/Time.idl /workspace/target_idl/builtin_interfaces/msg/
echo '       🟢 표준 IDL 복사 성공'
EOF

echo "▶ [2/3] 외부 사양서 파싱 및 원격 소스 다운로드 가동..."

# 4. 호스트 환경에 임시 샌드박스 빌드 트리 레이아웃 세팅
rm -rf "${TMP_BUILD_DIR}"
mkdir -p "${TMP_BUILD_DIR}"
: > "${PKG_LIST_FILE}"

echo "==> '${CONFIG_FILE}' 파일 분석 및 메시지 다운로드 중..."

while IFS= read -r line || [ -n "$line" ]; do
    [[ "$line" =~ ^[[:space:]]*# ]] && continue
    [[ -z "${line//[[:space:]]/}" ]] && continue

    MSG_FILE_NAME=$(echo "${line}" | cut -d':' -f1 | tr -d '\r[:space:]')
    URL=$(echo "${line}" | cut -d':' -f2- | tr -d '\r[:space:]')

    if [[ ! "${MSG_FILE_NAME}" == *.msg ]]; then
        MSG_FILE_NAME="${MSG_FILE_NAME}.msg"
    fi

    PKG_NAME=$(echo "${URL}" | sed -E 's#^.*/([^/]+)/msg/[^/]+$#\1#')
    if [[ -z "${PKG_NAME}" || "${PKG_NAME}" == "${URL}" ]]; then
        echo "       ❌ 패키지명 추출 실패! URL을 다시 확인하세요: ${URL}"
        exit 1
    fi

    PKG_DIR="${TMP_BUILD_DIR}/${PKG_NAME}"
    mkdir -p "${PKG_DIR}/msg"
    mkdir -p "${TARGET_DIR}/${PKG_NAME}/msg"

    if ! grep -qx "${PKG_NAME}" "${PKG_LIST_FILE}"; then
        echo "${PKG_NAME}" >> "${PKG_LIST_FILE}"
    fi

    echo "    -> [다운로드 시도] ${PKG_NAME}/${MSG_FILE_NAME}"
    if curl -sSfL "${URL}" -o "${PKG_DIR}/msg/${MSG_FILE_NAME}"; then
        echo "       🟢 다운로드 성공"
    else
        echo "       ❌ 다운로드 실패! URL을 다시 확인하세요: ${URL}"
        exit 1
    fi
done < "${CONFIG_FILE}"

while IFS= read -r PKG_NAME || [ -n "${PKG_NAME}" ]; do
    [ -z "${PKG_NAME}" ] && continue
    create_ros_interface_package "${TMP_BUILD_DIR}/${PKG_NAME}" "${PKG_NAME}"
done < "${PKG_LIST_FILE}"

# 5. Docker 가동: 다운로드된 전체 자산을 대상으로 colcon 일괄 빌드 돌리기
docker run --rm -i \
  -v "${SCRIPT_DIR}:/workspace" \
  ${DOCKER_IMAGE} \
  bash <<EOF
set -e
apt-get update -qq && apt-get install -y -qq ros-humble-rosidl-generator-dds-idl python3-colcon-common-extensions > /dev/null 2>&1

mkdir -p /tmp/ws/src
cp -r /workspace/.tmp_ws_src/. /tmp/ws/src/

cd /tmp/ws
source /opt/ros/humble/setup.bash
echo '    -> colcon 빌드 시작...'
colcon build 2>&1
echo '    -> colcon 빌드 완료'

echo '==> 생성된 IDL 아티팩트 유효성 검증 및 호스트 전송 중...'
while read -r PKG_NAME; do
    [ -z "\${PKG_NAME}" ] && continue
    IDL_COUNT=\$(find "/tmp/ws/build/\${PKG_NAME}" -name "*.idl" | wc -l)
    echo "    -> [\${PKG_NAME}] 발견된 IDL 파일 수: \${IDL_COUNT}"

    find "/tmp/ws/build/\${PKG_NAME}" -name "*.idl" | while read -r IDL_PATH; do
        IDL_NAME=\$(basename "\${IDL_PATH}")
        CLEAN_NAME=\$(echo "\${IDL_NAME}" | sed 's/_\.idl/\.idl/')

        if [ "\${CLEAN_NAME}" = "Header.idl" ]; then
            cp -v "\${IDL_PATH}" "/workspace/target_idl/std_msgs/msg/\${CLEAN_NAME}"
        elif [ "\${CLEAN_NAME}" = "Time.idl" ]; then
            cp -v "\${IDL_PATH}" "/workspace/target_idl/builtin_interfaces/msg/\${CLEAN_NAME}"
        else
            mkdir -p "/workspace/target_idl/\${PKG_NAME}/msg"
            cp -v "\${IDL_PATH}" "/workspace/target_idl/\${PKG_NAME}/msg/\${CLEAN_NAME}"
        fi
    done
done < /workspace/.tmp_ws_src/packages.txt

chown -R ${USER_UID}:${USER_GID} /workspace/target_idl
echo '    -> Docker 작업 완료'
EOF

# 6. 생성된 IDL 파일들을 상위 idl 폴더로 복사
IDL_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
echo "==> 생성된 IDL 파일들을 ${IDL_DIR} 폴더로 복사 중..."

while IFS= read -r PKG_NAME || [ -n "${PKG_NAME}" ]; do
    [ -z "${PKG_NAME}" ] && continue
    mkdir -p "${IDL_DIR}/${PKG_NAME}/msg"
    find "${TARGET_DIR}/${PKG_NAME}/msg" -name "*.idl" 2>/dev/null | while read -r IDL_PATH; do
        rm -f "${IDL_DIR}/$(basename "${IDL_PATH}")"
        cp "${IDL_PATH}" "${IDL_DIR}/${PKG_NAME}/msg/"
        echo "       🟢 복사 성공: ${PKG_NAME}/msg/$(basename "${IDL_PATH}")"
    done
done < "${PKG_LIST_FILE}"

mkdir -p "${IDL_DIR}/std_msgs/msg"
find "${TARGET_DIR}/std_msgs/msg" -name "*.idl" 2>/dev/null | while read -r IDL_PATH; do
    rm -f "${IDL_DIR}/$(basename "${IDL_PATH}")"
    cp "${IDL_PATH}" "${IDL_DIR}/std_msgs/msg/"
    echo "       🟢 복사 성공: std_msgs/msg/$(basename "${IDL_PATH}")"
done
mkdir -p "${IDL_DIR}/builtin_interfaces/msg"
find "${TARGET_DIR}/builtin_interfaces/msg" -name "*.idl" 2>/dev/null | while read -r IDL_PATH; do
    rm -f "${IDL_DIR}/$(basename "${IDL_PATH}")"
    cp "${IDL_PATH}" "${IDL_DIR}/builtin_interfaces/msg/"
    echo "       🟢 복사 성공: builtin_interfaces/msg/$(basename "${IDL_PATH}")"
done

# 7. 호스트에 생성했던 임시 빌드 디렉토리 및 target_idl 파편 깔끔하게 소거
rm -rf "${TMP_BUILD_DIR}"
rm -rf "${TARGET_DIR}"

echo "🟢 [3/3] DDS 인터페이스 빌드 아티팩트 생성 완료!"
echo "📂 출력 경로: $(cd .. && pwd)"
