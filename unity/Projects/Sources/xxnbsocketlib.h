﻿#pragma once

// 这个封装主要为 C# 服务. LUA 啥的直接引用具体实现的 .h

#ifdef _WIN32
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef xxnbsocketlib_EXPORTS
#define XXNBSOCKETLIB_API __declspec(dllexport)
#else
#define XXNBSOCKETLIB_API __declspec(dllimport)
#endif
#else
#define XXNBSOCKETLIB_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

	// 初始化网络系统( WSAStartup / signal ). 只需要一开始执行一次.
	XXNBSOCKETLIB_API void Init_Sock() noexcept;



	// 建 XxMemPool
	XXNBSOCKETLIB_API void* XxMemPool_New() noexcept;

	// 杀 XxMemPool( 在 XxNBSocket 之后再杀 )
	XXNBSOCKETLIB_API void XxMemPool_Delete(void* mp) noexcept;




	// 建 XxNBSocket( 传入 XxMemPool )
	XXNBSOCKETLIB_API void* XxNBSocket_New(void* mp) noexcept;

	// 杀 XxNBSocket
	XXNBSOCKETLIB_API void XxNBSocket_Delete(void* nbs) noexcept;

	// 设 ip & port
	XXNBSOCKETLIB_API void XxNBSocket_SetAddress(void* nbs, char* ip, uint16_t port) noexcept;

	// 开始连接. 可传入阻塞时长
	// 返回负数 表示出错. 0 表示没发生错误 但也没连上. 1 表示连接成功
	XXNBSOCKETLIB_API int XxNBSocket_Connect(void* nbs, int sec, int usec) noexcept;

	// 断开连接. 可传入延迟多少 ticks 断开. 0 立即断开.
	XXNBSOCKETLIB_API void XxNBSocket_Disconnect(void* nbs, int delayTicks) noexcept;

	// 帧驱动. 可传入阻塞时长( 仅对 Connecting 状态有效 )
	// 返回负数表示出错. 0 表示没发生错误
	XXNBSOCKETLIB_API int XxNBSocket_Update(void* nbs, int sec, int usec) noexcept;

	// 取当前状态. 
	// Disconnected,		// 0: 初始状态 / 出错 / 成功断开后
	// Connecting,			// 1: 执行 Connect 之后
	// Connected,			// 2: 握手并进入可收发状态之后
	// Disconnecting,		// 3: 执行 Disconnect( 延迟断开时长 ) 之后
	XXNBSOCKETLIB_API int XxNBSocket_GetState(void* nbs) noexcept;

	// 取当前状态已持续的 ticks( Disconnecting 除外 )
	XXNBSOCKETLIB_API int XxNBSocket_GetTicks(void* nbs) noexcept;

	// 发一段数据. 复制模式. 发送方不需要保持该数据不变.
	// 返回负数表示出错. 0 表示成功放入待发送队列或无数据可发. > 0 表示已立刻发送成功的长度. 
	// 如果有剩下部分, 会放入待发送队列, 在下次 Update 时继续发
	XXNBSOCKETLIB_API int XxNBSocket_Send(void* nbs, char* buf, int dataLen) noexcept;

	// 如果接收缓冲区有包, 将返回 buf 指针填充长度. 否则返回 null( 表示无数据可取 )
	// 流程: while( buf = PeekRecv( &dataLen ) ) {  ... PopRecv(); }
	XXNBSOCKETLIB_API char* XxNBSocket_PeekRecv(void* nbs, int* dataLen) noexcept;

	// 对于已处理的 PeekRecv 的数据, 用这个函数来弹出删掉, 以便继续 Peek 下一条.
	XXNBSOCKETLIB_API void XxNBSocket_PopRecv(void* nbs) noexcept;

#ifdef __cplusplus
}
#endif
