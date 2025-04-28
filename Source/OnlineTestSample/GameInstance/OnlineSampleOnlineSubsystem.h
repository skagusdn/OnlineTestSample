// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Online/Lobbies.h"
#include "Online/OnlineAsyncOpHandle.h"

#include "Online/OnlineServices.h"
#include "Online/Presence.h"
#include "Online/TitleFile.h"
#include "Online/Sessions.h"
#include "Online/Social.h"
#include "Online/UserInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSampleOnlineSubsystem.generated.h"


namespace UE::Online
{
	struct FLobbyJoined;
	
	struct FLobbyMemberLeft;
	struct FLobbyMemberJoined;

	struct FLobby;
	struct FCreateLobby;
	struct FAuthLogin;
}

class UOnlineUserInfo;
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineSampleOnlineSubsystem, Log, All);

USTRUCT(BlueprintType)
struct FBlueprintSessionInfo
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	FString SessionInfoString;

	UE::Online::FOnlineSessionId SessionId;
	
};

USTRUCT(BlueprintType)
struct FCreateLobbyRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadWrite)
	FName LobbyName;
	
	UPROPERTY(BlueprintReadWrite)
	TSoftObjectPtr<UWorld> LevelToTravel;
};


USTRUCT(BlueprintType)
struct FBlueprintLobbyInfo
{
	GENERATED_BODY()

	FBlueprintLobbyInfo(){};
	FBlueprintLobbyInfo(TSharedPtr<const UE::Online::FLobby> InLobby) : Lobby(InLobby), MaxMembers(InLobby.Get()->MaxMembers)
	, LobbyName(InLobby.Get()->LocalName)
	{
		for(auto& Member : InLobby.Get()->Members)
		{
			Members.Add(Member.Key.GetHandle());
		}
	};
	
	TSharedPtr<const UE::Online::FLobby> Lobby;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxMembers = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> Members;

	UPROPERTY(BlueprintReadOnly)
	FName LobbyName;
};

USTRUCT(BlueprintType)
struct FBlueprintLobbyMemberInfo
{
	GENERATED_BODY()

	FBlueprintLobbyMemberInfo(){};
	FBlueprintLobbyMemberInfo(const UE::Online::FLobbyMember& InLobbyMember, bool IsLobbyOwner) : LobbyMember( &InLobbyMember ), bIsLobbyOwner(IsLobbyOwner)
	, bIsLocalMember(InLobbyMember.bIsLocalMember){};
	
	const UE::Online::FLobbyMember* LobbyMember;

	bool bIsLobbyOwner = false;
	bool bIsLocalMember = false;
};

USTRUCT(BlueprintType)
struct FBlueprintUserInfo
{
	GENERATED_BODY()

	FBlueprintUserInfo(){};
	FBlueprintUserInfo(UE::Online::FUserInfo* InUserInfo) : User(InUserInfo), UserId(InUserInfo->AccountId.GetHandle())
	, DisplayName(InUserInfo->DisplayName){};
	
	const UE::Online::FUserInfo* User;

	UPROPERTY(BlueprintReadOnly)
	int32 UserId = -1;
	
	UPROPERTY(BlueprintReadOnly)
	FString DisplayName;
	

};

USTRUCT(BlueprintType)
struct FBlueprintPresenceInfo
{
	GENERATED_BODY()

	FBlueprintPresenceInfo(){};

	const UE::Online::FUserPresence* Presence = nullptr;
};

USTRUCT(BlueprintType)
struct FBlueprintFriendInfo
{
	GENERATED_BODY()

	FBlueprintFriendInfo(){};
	FBlueprintFriendInfo(UE::Online::FFriend* InFriend) : Friend(InFriend), FriendId(InFriend->FriendId.GetHandle())
	, DisplayName(InFriend->DisplayName), Nickname(InFriend->Nickname){};
	
	const UE::Online::FFriend* Friend = nullptr;;

	UPROPERTY(BlueprintReadOnly)
	int32 FriendId = -1;
	
	UPROPERTY(BlueprintReadOnly)
	FString DisplayName;
	
	UPROPERTY(BlueprintReadOnly)
	FString Nickname;

	

};

DECLARE_MULTICAST_DELEGATE_OneParam(FLoginComplete, bool bSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoginComplete_Dynamic, bool, bSucceeded);

DECLARE_MULTICAST_DELEGATE_OneParam(FCreateLobbyComplete, bool bSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCreateLobbyComplete_Dynamic, bool, bSucceeded);

DECLARE_MULTICAST_DELEGATE_OneParam(FLobbyInfoUpdated, FBlueprintLobbyInfo LobbyInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLobbyInfoUpdated_Dynamic, FBlueprintLobbyInfo, LobbyInfo);

DECLARE_MULTICAST_DELEGATE_OneParam(FFindLobbiesComplete, bool bSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFindLobbiesComplete_Dynamic, bool, bSucceeded);

DECLARE_MULTICAST_DELEGATE_TwoParams(FJoinLobbyComplete, bool bSucceeded, FBlueprintLobbyInfo LobbyInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJoinLobbyComplete_Dynamic, bool, bSucceeded, FBlueprintLobbyInfo, LobbyInfo);

DECLARE_MULTICAST_DELEGATE_OneParam(FGetFriendsComplete, bool bSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGetFriendsComplete_Dynamic, bool, bSucceeded);


/**
 * 
 */
UCLASS()
class ONLINETESTSAMPLE_API UOnlineSampleOnlineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
 
public:
 
	////////////////////////////////////////////////////////
	/// OnlineSampleOnlineSubsystem Init/Deinit 
 
	/** 서브시스템 생성 여부를 결정하기 위해 호출됨 */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
 
	/** 게임 인스턴스 서브시스템을 초기화하기 위해 호출됨 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 
	/** 게임 인스턴스 서브시스템의 초기화를 해제하기 위해 호출됨 */
	virtual void Deinitialize() override;
 
	/** 게임 인스턴스 서브시스템에 로컬 온라인 사용자를 등록하기 위해 호출됨 */
	void RegisterLocalOnlineUser(FPlatformUserId PlatformUserId);
 
	/** 이 플랫폼 사용자 ID에 대한 온라인 사용자 정보를 얻기 위해 호출됨 */
	TObjectPtr<const UOnlineUserInfo> GetOnlineUserInfo(FPlatformUserId PlatformUserId);

	//-----------------------------------------------------------------------------------------
	//세션 생성
	UFUNCTION(BlueprintCallable, DisplayName="Create Session")
	void K2_CreateSession(int32 MaxPlayer, bool IsLan, FPlatformUserId PlatformUserId);
	void CreateSession(UE::Online::FCreateSession::Params SessionParams, FPlatformUserId PlatformUserId);
	void FindSessions(UE::Online::FFindSessions::Params SessionFindParams);

	UFUNCTION(BlueprintCallable, DisplayName="Create Lobby")
	void K2_CreateLobby(ULocalPlayer* LocalPlayer, const FCreateLobbyRequest & Request);
	void CreateLobby(ULocalPlayer* LocalPlayer, const FCreateLobbyRequest & Request);

	UFUNCTION(BlueprintCallable, DisplayName="Find Lobbies")
	void K2_FindLobbies(ULocalPlayer* LocalPlayer);

	//UFUNCTION(BlueprintCallable)
	//void FindLobbiesByUser(ULocalPlayer* LocalPlayer, const FBlueprintUserInfo& UserInfo);

	UFUNCTION(BlueprintCallable)
	void FindLobbiesByUser(ULocalPlayer* LocalPlayer, const FBlueprintFriendInfo& FriendInfo);
	void FindLobbies(ULocalPlayer* LocalPlayer, UE::Online::FFindLobbies::Params FindLobbyParams);
	
	
	UFUNCTION(BlueprintCallable, DisplayName="Join Lobby")
	void K2_JoinLobby(ULocalPlayer* LocalPlayer, const FBlueprintLobbyInfo& LobbyInfoToJoin);
	UFUNCTION(BlueprintCallable)
	void JoinFriendLobby(ULocalPlayer* LocalPlayer, const FBlueprintFriendInfo& FriendInfo);
	void JoinLobby(ULocalPlayer* LocalPlayer, const FBlueprintLobbyInfo& LobbyInfoToJoin);

	UFUNCTION(BlueprintCallable, DisplayName="Leave Lobby")
	void K2_LeaveLobby(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo);
	void LeaveLobby(ULocalPlayer* LocalPlayer, UE::Online::FLobbyId LobbyId);
	void LeaveLobby(FPlatformUserId PlatformUserId, UE::Online::FLobbyId LobbyId);
	
	UFUNCTION(BlueprintCallable)
	void InitFriendsInfo(ULocalPlayer* LocalPlayer);

	UFUNCTION(BlueprintCallable)
	void StartGameFromLobby(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo);
	void TravelToLobby(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo);
	
	// UFUNCTION(BlueprintCallable, DisplayName="Get Friends")
	// void K2_GetFriends(ULocalPlayer* LocalPlayer);
	void GetFriends(ULocalPlayer* LocalPlayer);

	// UFUNCTION(BlueprintCallable, DisplayName="Query Presence")
	// void K2_QueryPresence(ULocalPlayer* LocalPlayer);
	void QueryPresence(ULocalPlayer* LocalPlayer, UE::Online::FAccountId TargetId, bool bListenToChanges);
	
	// UFUNCTION(BlueprintCallable, DisplayName="Get User Info")
	// void K2_GetUserInfo(ULocalPlayer* LocalPlayer, );
	void GetUserInfo(ULocalPlayer* LocalPlayer, TArray<UE::Online::FAccountId> TargetUsers);

	
	
	//로그인
	void Login(FPlatformUserId PlatformUserId);
	void Logout();
	bool IsLoggedIn(ULocalPlayer* LocalPlayer);

	//임시 함수들
	
	
	// UFUNCTION(BlueprintCallable)
	// TArray<FBlueprintSessionInfo> GetFoundSessions() {return FoundSessions;};
		
	UFUNCTION(BlueprintCallable)
	void K2_FindSessions(APlayerController* PlayerController, int32 MaxResults, bool bUseLan);

	/// Events
	
	FLoginComplete OnLoginCompleteEvent;
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Login Complete"))
	FLoginComplete_Dynamic K2_OnLoginCompleteEvent;
	
	FCreateLobbyComplete OnCreateLobbyCompleteEvent;
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Create Lobby Complete"))
	FCreateLobbyComplete_Dynamic K2_OnCreateLobbyCompleteEvent;

	FJoinLobbyComplete OnJoinLobbyCompleteEvent;
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Join Lobby Complete"))
	FJoinLobbyComplete_Dynamic K2_OnJoinLobbyCompleteEvent;
	
	FLobbyInfoUpdated OnLobbyInfoUpdatedEvent;
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Lobby Info Updated"))
	FLobbyInfoUpdated_Dynamic K2_OnLobbyInfoUpdatedEvent;

	FFindLobbiesComplete OnFindLobbiesCompleteEvent;
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Find Lobbies Complete"))
	FFindLobbiesComplete_Dynamic K2_OnFindLobbiesCompleteEvent;

	FGetFriendsComplete OnGetFriendsCompleteEvent;
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Get Friends Complete"))
	FGetFriendsComplete_Dynamic K2_OnGetFriendsCompleteEvent;

	UPROPERTY(BlueprintReadWrite, meta=(AllowedTypes="World"))
	FPrimaryAssetId MapID;

	UPROPERTY(BlueprintReadWrite)
	TSoftObjectPtr<UWorld> MyMap;//
	
protected:
 
	struct FOnlineServicesInfo 
	{
		/** 온라인 서비스 포인터 - 이 포인터를 통해 인터페이스에 액세스합니다 */
		UE::Online::IOnlineServicesPtr OnlineServices = nullptr;
 
		/** 인터페이스 포인터입니다 */
		UE::Online::IAuthPtr AuthInterface = nullptr;
		UE::Online::ITitleFilePtr TitleFileInterface = nullptr;
		UE::Online::ISessionsPtr SessionsInterface = nullptr;//
		UE::Online::ILobbiesPtr LobbiesInterface = nullptr;//
		UE::Online::ISocialPtr SocialInterface = nullptr;//
		UE::Online::IUserInfoPtr UserInfoInterface = nullptr;//
		UE::Online::IPresencePtr PresenceInterface = nullptr;//
		
		
		/** 온라인 서비스 구현입니다 */
		UE::Online::EOnlineServices OnlineServicesType = UE::Online::EOnlineServices::None;
 
		/** 타이틀 파일 콘텐츠입니다 */
		UE::Online::FTitleFileContents TitleFileContent;
 
		/** 구조체를 초기 세팅으로 리셋합니다 */
		void Reset()
		{
			OnlineServices.Reset();
			AuthInterface.Reset();
			TitleFileInterface.Reset();
			//
			SessionsInterface.Reset();
			LobbiesInterface.Reset();
			SocialInterface.Reset();
			UserInfoInterface.Reset();
			PresenceInterface.Reset();
			//
			OnlineServicesType = UE::Online::EOnlineServices::None;
		}
	};
 
	////////////////////////////////////////////////////////
	/// 온라인 서비스 초기화
 
	/** 관련 온라인 서비스 포인터가 포함된 내부 구조체에 대한 포인터입니다 */
	FOnlineServicesInfo* OnlineServicesInfoInternal = nullptr;
 
	/** 온라인 서비스 및 인터페이스 포인터를 초기화하기 위해 호출됨 */
	void InitializeOnlineServices();
 
	
	////////////////////////////////////////////////////////
	
	// 로그인 이벤트 처리...는 나중에 구현.
	void HandleLogin(const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& LoginResult);
	void HandleCreateLobby(const UE::Online::TOnlineResult<UE::Online::FCreateLobby>& CreateLobbyResult);
	// 세션 생성 비동기 이벤트 처리.
	void HandleCreateSession(const UE::Online::TOnlineResult<UE::Online::FCreateSession>& CreateSessionResult );

	void BindLobbyUpdatedEvents();
	const void NotifyLobbyUpdated();
	void HandleMemberJoinedLobby(const UE::Online::FLobbyMemberJoined& Info);
	void HandleMemberLeftLobby(const UE::Online::FLobbyMemberLeft& Info);
	void HandleJoinedLobby(const UE::Online::FLobbyJoined& Info);
	void HandleLeftLobby(const UE::Online::FLobbyLeft& Info);
	void HandleLobbyAttributeChanged(const UE::Online::FLobbyAttributesChanged& Info);

	void HandleFindLobbies(const UE::Online::TOnlineResult<UE::Online::FFindLobbies>& FindLobbiesResult);
	void HandleJoinLobby(const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& JoinLobbyResult);

	void HandleGetFriends(const UE::Online::TOnlineResult<UE::Online::FGetFriends>& GetFriendsResult);
	void HandleGetUserInfo(const UE::Online::TOnlineResult<UE::Online::FGetUserInfo>& GetUserInfoResult);
	
	void BindPresenceUpdatedEvents();
	void HandleFriendsUpdated(const UE::Online::FPresenceUpdated&);
	const void NotifyPresenceUpdated();
	
	void AdjustLobbyAfterStart(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo);
	
	//데이터
	TArray<UE::Online::FOnlineEventDelegateHandle> LobbyMemberChangeEvent_Handles;
	TArray<UE::Online::FOnlineEventDelegateHandle> PresenceUpdatedEvent_Handles;
	

	UPROPERTY(BlueprintReadOnly)
	TArray<FBlueprintSessionInfo> FoundSessions;
	UPROPERTY(BlueprintReadOnly)
	TArray<FBlueprintLobbyInfo> FoundLobbies;
	UPROPERTY(BlueprintReadOnly)
	TArray<FBlueprintUserInfo> FoundUsers;
	UPROPERTY(BlueprintReadOnly)
	FBlueprintLobbyInfo JoinedLobby;
	
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FBlueprintFriendInfo> FoundFriends;
	
	TSharedPtr<const UE::Online::FLobby> CreatedLobby = nullptr;
	//TSharedPtr<const UE::Online::FLobby> JoinedLobby = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FBlueprintLobbyMemberInfo LocalPlayerLobbyMemberInfo;
	
	
	
	
	////////////////////////////////////////////////////////
    /// 온라인 사용자 정보
 
	/** 이 사용자에 대한 UOnlineUserInfo 오브젝트를 생성하기 위해 호출됨 */
	TObjectPtr<UOnlineUserInfo> CreateOnlineUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, UE::Online::FAccountId AccountId, UE::Online::EOnlineServices Services);
	
	/** CreateOnlineUserInfo로 생성 후 OnlineUserInfos 맵에 사용자를 등록하고 사용자를 추가하기 위해 호출됨 */
	TObjectPtr<UOnlineUserInfo> CreateAndRegisterUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, UE::Online::FAccountId AccountId, UE::Online::EOnlineServices Services);
 
	/** 각 로컬 사용자에 대한 정보입니다 */
	UPROPERTY()
	TMap<int32, TObjectPtr<const UOnlineUserInfo>> OnlineUserInfos;
 
	/** 액세스하기 위해 UOnlineUserInfo 클래스를 friend 선언합니다 */
	friend UOnlineUserInfo;
};
 
UCLASS()
class ONLINETESTSAMPLE_API UOnlineUserInfo : public UObject
{
 
	GENERATED_BODY()
 
public:
 
	UOnlineUserInfo();
 
	////////////////////////////////////////////////////////
    /// 온라인 사용자 필드
 
	int32 LocalUserIndex = -1;
	FPlatformUserId PlatformUserId;
	UE::Online::FAccountId AccountId;
	UE::Online::EOnlineServices Services = UE::Online::EOnlineServices::None;
 
	////////////////////////////////////////////////////////
	/// 온라인 사용자 로깅/디버깅 함수
 
	/** OnlineUserInfo를 스트링으로 가져오기 위해 호출됨 */
	const FString DebugInfoToString();
 
	friend UOnlineSampleOnlineSubsystem;
};