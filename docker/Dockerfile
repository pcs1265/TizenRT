FROM tizenrt/tizenrt:latest

# 기존 gcc-arm-none-eabi-gcc 10.3.1 제거
RUN apt-get update && apt-get remove -y gcc-arm-none-eabi

# 필요 패키지 설치 (tar, xz-utils 등)
# RUN apt-get update && apt-get install -y tar xz-utils

# 사전 다운로드한 toolchain 복사
COPY arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz /tmp/

# 압축 해제 및 설치
RUN tar -xf /tmp/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz -C /opt \
    && rm /tmp/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz

# PATH 설정
ENV PATH="/opt/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi/bin:${PATH}"

# 확인용
RUN arm-none-eabi-gcc --version
