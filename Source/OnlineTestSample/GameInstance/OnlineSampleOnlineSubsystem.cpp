// Fill out your copyright notice in the Description page of Project Settings.


#include "OnlineSampleOnlineSubsystem.h"

#include "Engine/AssetManager.h"
//#include "GameFramework/GameSession.h"
#include "Online/OnlineResult.h"
#include "Online/Auth.h"
#include "Online/Lobbies.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/Presence.h"
#include "Online/UserInfo.h"


DEFINE_LOG_CATEGORY(LogOnlineSampleOnlineSubsystem);
 
/// <summary>
///이 서브시스템을 생성할지 여부입니다. 단순화를 위해 서브시스템은 오직
///		서버가 아닌 클라이언트 및 독립형 게임에서만 생성됩니다. 이 함수는
///		종종 서버 또는 클라이언트로 서브시스템 생성을 제한하는 데 사용됩니다.
///		사용 전에 서브시스템 null 체크를 해야 합니다!
/// </summary>
/// <param name="Outer"></param>
/// <returns>이 서브시스템을 생성할지 여부를 정하는 boolean입니다</returns>
bool UOnlineSampleOnlineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_SERVER
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer);
#endif
}
 
/// <summary>
/// 게임 인스턴스가 초기화된 뒤에 호출되는 초기화입니다
/// </summary>
/// <param name="Collection">게임 인스턴스에 의해 초기화되는 서브시스템의 컬렉션입니다</param>
void UOnlineSampleOnlineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Log, TEXT("OnlineSampleOnlineSubsystem initialized."));
	Super::Initialize(Collection);
 
	// 온라인 서비스를 초기화합니다
	InitializeOnlineServices();

	// 로비 업데이트 될때 호출될 콜백 함수 바인드. 우선 멤버 관련만 바인드 해놨으므.
	BindLobbyUpdatedEvents();
	// 현재 상태(친구창) 업데이트 될 때 호출될 함수 바인드. 우선 친구쪽. 
	BindPresenceUpdatedEvents();
}
 
/// <summary>
/// 게임 인스턴스가 초기화 해제되거나 종료되기 전에 호출되는 초기화 해제입니다
/// </summary>
void UOnlineSampleOnlineSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("OnlineSampleOnlineSubsystem deinitialized."));

	// 로그아웃 ㄱㄱ
	Logout();
	// 이벤트 핸들 바인딩을 해제하고 구조체 정보를 리셋합니다
	OnlineServicesInfoInternal->Reset();
 
	// 부모 클래스 초기화를 해제합니다
	Super::Deinitialize();
}

void UOnlineSampleOnlineSubsystem::HandleCreateLobby(
	const UE::Online::TOnlineResult<UE::Online::FCreateLobby>& CreateLobbyResult)
{
	using namespace UE::Online;
	
	bool IsSucceeded = false;
	if(CreateLobbyResult.IsOk())
	{
		IsSucceeded = true;
		CreatedLobby = CreateLobbyResult.GetOkValue().Lobby;
		JoinedLobby = FBlueprintLobbyInfo(CreatedLobby);
		
		UE_LOG(LogTemp, Warning, TEXT("Create Lobby Completed"));

		for(auto& Attribute: CreatedLobby.Get()->Attributes)
		{
			UE_LOG(LogTemp, Display, TEXT("Attribute - %s"), *Attribute.Key.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Create Lobby Failed : %s"), *CreateLobbyResult.GetErrorValue().GetLogString());
	}
	
	OnCreateLobbyCompleteEvent.Broadcast(IsSucceeded);
	K2_OnCreateLobbyCompleteEvent.Broadcast(IsSucceeded);

	NotifyLobbyUpdated();
}

void UOnlineSampleOnlineSubsystem::HandleFindLobbies(
	const UE::Online::TOnlineResult<UE::Online::FFindLobbies>& FindLobbiesResult)
{
	using namespace UE::Online;
	
	bool IsSucceeded = false;
	if(FindLobbiesResult.IsOk())
	{
		IsSucceeded = true;
		FoundLobbies.Empty();
		for(TSharedRef<const FLobby> Lobby : FindLobbiesResult.GetOkValue().Lobbies)
		{
			FoundLobbies.Add(FBlueprintLobbyInfo(Lobby));
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Find Lobby Completed"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Find Lobby Failed : %s"), *FindLobbiesResult.GetErrorValue().GetLogString());
	}
	
	OnFindLobbiesCompleteEvent.Broadcast(IsSucceeded);
	K2_OnFindLobbiesCompleteEvent.Broadcast(IsSucceeded);
}

void UOnlineSampleOnlineSubsystem::HandleJoinLobby(
	const UE::Online::TOnlineResult<UE::Online::FJoinLobby>& JoinLobbyResult)
{
	using namespace UE::Online;
	
	bool IsSucceeded = false;
	FBlueprintLobbyInfo LobbyInfo;
	
	if(JoinLobbyResult.IsOk())
	{
		IsSucceeded = true;
		LobbyInfo = FBlueprintLobbyInfo(JoinLobbyResult.GetOkValue().Lobby);

		//우선 임시로 로컬 플레이어 이렇게 얻기.
		
		TravelToLobby(GetWorld()->GetFirstLocalPlayerFromController(), LobbyInfo);
		
		UE_LOG(LogTemp, Warning, TEXT("Join Lobby Completed"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Join Lobby Failed : %s"), *JoinLobbyResult.GetErrorValue().GetLogString());
	}
	
	OnJoinLobbyCompleteEvent.Broadcast(IsSucceeded, LobbyInfo);
	K2_OnJoinLobbyCompleteEvent.Broadcast(IsSucceeded, LobbyInfo);
	
	NotifyLobbyUpdated();
}

void UOnlineSampleOnlineSubsystem::HandleGetFriends(
	const UE::Online::TOnlineResult<UE::Online::FGetFriends>& GetFriendsResult)
{
	using namespace UE::Online;

	FoundFriends.Empty();
	
	if(GetFriendsResult.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("Get Friends Success"));
		
		for(TSharedRef<FFriend> Friend : GetFriendsResult.GetOkValue().Friends)
		{
			FoundFriends.Emplace(Friend.Get().FriendId.GetHandle(), FBlueprintFriendInfo(&Friend.Get()));
			UE_LOG(LogTemp, Display, TEXT("Found Friend DisplayName : %s"), *Friend.Get().DisplayName);//
			UE_LOG(LogTemp, Display, TEXT("Found Friend NickName: %s"), *Friend.Get().Nickname);//
			UE_LOG(LogTemp, Display, TEXT("Found Friend Id: %d"), Friend.Get().FriendId.GetHandle());//
			//
		}
		
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Get Friends Failed : %s"), *GetFriendsResult.GetErrorValue().GetLogString());
	}

	OnGetFriendsCompleteEvent.Broadcast(GetFriendsResult.IsOk());
	K2_OnGetFriendsCompleteEvent.Broadcast(GetFriendsResult.IsOk());
	
}

void UOnlineSampleOnlineSubsystem::HandleGetUserInfo(
	const UE::Online::TOnlineResult<UE::Online::FGetUserInfo>& GetUserInfoResult)
{
	using namespace UE::Online;

	if(GetUserInfoResult.IsOk())
	{
		
		FoundUsers.Add(&GetUserInfoResult.GetOkValue().UserInfo.Get());
		UE_LOG(LogTemp, Warning, TEXT("Get User Info Success"));

		//
		UE_LOG(LogTemp, Display, TEXT("User Info ID : %d"), GetUserInfoResult.GetOkValue().UserInfo.Get().AccountId.GetHandle());
		UE_LOG(LogTemp, Display, TEXT("User Info DisplayName : %s"), *GetUserInfoResult.GetOkValue().UserInfo.Get().DisplayName);
		//
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Get User Info Failed : %s"), *GetUserInfoResult.GetErrorValue().GetLogString());
	}
}

void UOnlineSampleOnlineSubsystem::BindPresenceUpdatedEvents()
{
	using namespace UE::Online;

	if(IPresencePtr PresenceInterface = OnlineServicesInfoInternal->PresenceInterface)
	{
		PresenceUpdatedEvent_Handles.Add(PresenceInterface->OnPresenceUpdated().Add(this, &ThisClass::HandleFriendsUpdated));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Online Services Presence Interface is NOT VALID"));
	}
	
}

void UOnlineSampleOnlineSubsystem::HandleFriendsUpdated(const UE::Online::FPresenceUpdated& DataUpdated)
{
	using namespace UE::Online;

	const FUserPresence& UpdatedPresence =  DataUpdated.UpdatedPresence.Get();
	UE_LOG(LogTemp, Warning, TEXT("User %d Has Been Updated"), UpdatedPresence.AccountId.GetHandle());

	if(FoundFriends.Contains(UpdatedPresence.AccountId.GetHandle()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Testing-%d"), FoundFriends.Find(UpdatedPresence.AccountId.GetHandle())->FriendId);	
	}
	
	
}

void UOnlineSampleOnlineSubsystem::AdjustLobbyAfterStart(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo)
{
	using namespace UE::Online;
	
	if(ILobbiesPtr LobbiesInterface = OnlineServicesInfoInternal->LobbiesInterface)
	{
		FModifyLobbyAttributes::Params ModifyLobbyParams;
		ModifyLobbyParams.UpdatedAttributes.Emplace(FName(TEXT("MATCHSTATE")), FString(TEXT("Starting")));
		ModifyLobbyParams.LobbyId = LobbyInfo.Lobby->LobbyId;
		ModifyLobbyParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
		
		LobbiesInterface->ModifyLobbyAttributes(MoveTemp(ModifyLobbyParams)).OnComplete([this, LocalPlayer, LobbyInfo, LobbiesInterface]
			(TOnlineResult<FModifyLobbyAttributes> ModifyLobbyAttributesResult)
		{
			if(ModifyLobbyAttributesResult.IsOk())
			{
				FModifyLobbyMemberAttributes::Params ModifyLobbyMemberParams;
				ModifyLobbyMemberParams.LobbyId = LobbyInfo.Lobby->LobbyId;
				ModifyLobbyMemberParams.UpdatedAttributes.Emplace(FName(TEXT("MATCHSTATE")), FString(TEXT("Starting")));
				ModifyLobbyMemberParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
				
				LobbiesInterface->ModifyLobbyMemberAttributes(MoveTemp(ModifyLobbyMemberParams)).OnComplete([this](TOnlineResult<FModifyLobbyMemberAttributes> ModifyLobbyMemberAttributesResult)
				{
					if(ModifyLobbyMemberAttributesResult.IsOk())
					{
						// if(MyMap.IsValid())
						// {
							
						// }
						// else
						// {
						// 	UE_LOG(LogTemp, Error, TEXT("Map is invalid"));
						// }
						
					}
				});
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Modify Lobby Attributes Failed : %s"), *ModifyLobbyAttributesResult.GetErrorValue().GetLogString());
			}
		});
		
	}
}

void UOnlineSampleOnlineSubsystem::HandleCreateSession(
	const UE::Online::TOnlineResult<UE::Online::FCreateSession>& CreateSessionResult)
{
	using namespace UE::Online;

	if(CreateSessionResult.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("Session Creation Success"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Session Creation Failed : %s"), *CreateSessionResult.GetErrorValue().GetLogString());
	}
}

void UOnlineSampleOnlineSubsystem::BindLobbyUpdatedEvents()
{
	using namespace UE::Online;
	
	if(ILobbiesPtr LobbiesInterface = OnlineServicesInfoInternal->LobbiesInterface)
	{
		LobbyMemberChangeEvent_Handles.Add(LobbiesInterface->OnLobbyMemberJoined().Add(this, &ThisClass::HandleMemberJoinedLobby));
		LobbyMemberChangeEvent_Handles.Add(LobbiesInterface->OnLobbyMemberLeft().Add(this, &ThisClass::HandleMemberLeftLobby));
		LobbyMemberChangeEvent_Handles.Add(LobbiesInterface->OnLobbyJoined().Add(this, &ThisClass::HandleJoinedLobby));
		LobbyMemberChangeEvent_Handles.Add(LobbiesInterface->OnLobbyLeft().Add(this, &ThisClass::HandleLeftLobby));
		LobbyMemberChangeEvent_Handles.Add(LobbiesInterface->OnLobbyAttributesChanged().Add(this, &ThisClass::HandleLobbyAttributeChanged));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Online Services Lobbies Interface is NOT VALID"));
	}
}

const void UOnlineSampleOnlineSubsystem::NotifyLobbyUpdated()
{
	UE_LOG(LogTemp, Display, TEXT("NotifyLobbyUpdated"));
	
	OnLobbyInfoUpdatedEvent.Broadcast(JoinedLobby);
	K2_OnLobbyInfoUpdatedEvent.Broadcast(JoinedLobby);
}

void UOnlineSampleOnlineSubsystem::HandleMemberJoinedLobby(const UE::Online::FLobbyMemberJoined& Info)
{
	UE_LOG(LogTemp, Display, TEXT("HandleMemberJoinedLobby"));
	
	JoinedLobby = FBlueprintLobbyInfo(Info.Lobby);
	NotifyLobbyUpdated();
}

void UOnlineSampleOnlineSubsystem::HandleMemberLeftLobby(const UE::Online::FLobbyMemberLeft& Info)
{
	UE_LOG(LogTemp, Display, TEXT("HandleMemberLeftLobby"));
	
	JoinedLobby = FBlueprintLobbyInfo(Info.Lobby);
	NotifyLobbyUpdated();
}

void UOnlineSampleOnlineSubsystem::HandleJoinedLobby(const UE::Online::FLobbyJoined& Info)
{
	using namespace UE::Online;
	
	UE_LOG(LogTemp, Display, TEXT("Handle Joined Lobby"));

	JoinedLobby = FBlueprintLobbyInfo(Info.Lobby);
	
	FAccountId LocalPlayerAccountId = GetOnlineUserInfo( GetWorld()->GetFirstLocalPlayerFromController()->GetPlatformUserId())->AccountId;
	if( Info.Lobby.Get().Members.Contains(LocalPlayerAccountId))
	{
		const FLobbyMember LobbyMember = (Info.Lobby.Get().Members.Find(LocalPlayerAccountId))->Get();
		LocalPlayerLobbyMemberInfo = FBlueprintLobbyMemberInfo(LobbyMember, Info.Lobby.Get().OwnerAccountId == LocalPlayerAccountId);

	}
	
	NotifyLobbyUpdated();
}

void UOnlineSampleOnlineSubsystem::HandleLeftLobby(const UE::Online::FLobbyLeft& Info)
{
	UE_LOG(LogTemp, Display, TEXT("Handle Left Lobby"));

	JoinedLobby = FBlueprintLobbyInfo();
	CreatedLobby = nullptr;
	LocalPlayerLobbyMemberInfo = FBlueprintLobbyMemberInfo();
		
	NotifyLobbyUpdated();
}

void UOnlineSampleOnlineSubsystem::HandleLobbyAttributeChanged(const UE::Online::FLobbyAttributesChanged& Info)
{
	using namespace UE::Online;
	
	UE_LOG(LogTemp, Display, TEXT("Handle Lobby Attributes Change"));

	// // 로비에서 게임이 시작되는 경우.
	// if(Info.ChangedAttributes.Contains(FName(TEXT("MATCHSTATE"))))
	// {
	// 	FString PrevMatchState = Info.ChangedAttributes.Find(FName(TEXT("MATCHSTATE")))->Key.GetString();
	// 	FString PostMatchState = Info.ChangedAttributes.Find(FName(TEXT("MATCHSTATE")))->Value.GetString();
	//
	// 	if(PrevMatchState.Equals(FString(TEXT("Waiting")))  && PostMatchState.Equals(FString(TEXT("Starting")))
	// 		&& !LocalPlayerLobbyMemberInfo.bIsLobbyOwner)
	// 	{
	// 		TravelToLobby(GetWorld()->GetFirstLocalPlayerFromController(), JoinedLobby);
	// 	}else
	// 	{
	// 		if(!PrevMatchState.Equals(FString(TEXT("Waiting"))))
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("Testing Something. Erase This LOG Later1"));
	// 		}
	// 		if(!PostMatchState.Equals(FString(TEXT("Starting"))))
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("Testing Something. Erase This LOG Later2"));
	// 		}
	// 		if(LocalPlayerLobbyMemberInfo.bIsLobbyOwner)
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("Testing Something. Erase This LOG Later3"));
	// 		}
	// 		
	// 		
	// 	}
	// }
	
	for(auto& Tuple : Info.ChangedAttributes)
	{
		UE_LOG(LogTemp, Display, TEXT("Changed Attribute - %s , %s"), *Tuple.Value.Key.ToLogString(), *Tuple.Value.Value.ToLogString());
	}
	//
	NotifyLobbyUpdated();
}


/// <summary>
/// 온라인 서비스를 초기화합니다.
///		서비스에 대한 포인터를 가져옵니다
///		인터페이스에 대한 포인터를 가져옵니다
///		이벤트 핸들을 추가합니다
///		포인터 유효성을 확인합니다
/// </summary>
void UOnlineSampleOnlineSubsystem::InitializeOnlineServices()
{
	OnlineServicesInfoInternal = new FOnlineServicesInfo();
 
	// 서비스 포인터를 초기화합니다
	OnlineServicesInfoInternal->OnlineServices = UE::Online::GetServices();
	check(OnlineServicesInfoInternal->OnlineServices.IsValid());
 
	// 서비스 타입을 검증합니다
	OnlineServicesInfoInternal->OnlineServicesType = OnlineServicesInfoInternal->OnlineServices->GetServicesProvider();
	if (OnlineServicesInfoInternal->OnlineServices.IsValid())
	{
		// 인터페이스 포인터를 초기화합니다
		OnlineServicesInfoInternal->AuthInterface = OnlineServicesInfoInternal->OnlineServices->GetAuthInterface();
		check(OnlineServicesInfoInternal->AuthInterface.IsValid());
 
		OnlineServicesInfoInternal->TitleFileInterface = OnlineServicesInfoInternal->OnlineServices->GetTitleFileInterface();
		check(OnlineServicesInfoInternal->TitleFileInterface.IsValid());

		//자 이번엔 내가 따라서 세션 인터페이스를 추가해보자.
		OnlineServicesInfoInternal->SessionsInterface = OnlineServicesInfoInternal->OnlineServices->GetSessionsInterface();
		check(OnlineServicesInfoInternal->SessionsInterface.IsValid());

		OnlineServicesInfoInternal->LobbiesInterface = OnlineServicesInfoInternal->OnlineServices->GetLobbiesInterface();
		check(OnlineServicesInfoInternal->LobbiesInterface.IsValid());

		OnlineServicesInfoInternal->SocialInterface = OnlineServicesInfoInternal->OnlineServices->GetSocialInterface();
		check(OnlineServicesInfoInternal->SocialInterface.IsValid());

		OnlineServicesInfoInternal->UserInfoInterface = OnlineServicesInfoInternal->OnlineServices->GetUserInfoInterface();
		check(OnlineServicesInfoInternal->UserInfoInterface.IsValid());

		OnlineServicesInfoInternal->PresenceInterface = OnlineServicesInfoInternal->OnlineServices->GetPresenceInterface();
		check(OnlineServicesInfoInternal->PresenceInterface.IsValid());
		
		
	}
	else {
		UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Error: Failed to initialize services."));
	}
}
 

/// <summary>
/// 로컬 레지스트리 OnlineUserInfos에 온라인 사용자를 등록합니다
/// </summary>
/// <param name="PlatformUserId">등록할 사용자의 플랫폼 사용자 ID입니다</param>
void UOnlineSampleOnlineSubsystem::RegisterLocalOnlineUser(FPlatformUserId PlatformUserId)
{
	using namespace UE::Online;
 
	FAuthGetLocalOnlineUserByPlatformUserId::Params GetUserParams;
	//GetUserParams.
	GetUserParams.PlatformUserId = PlatformUserId;
	if (OnlineServicesInfoInternal->AuthInterface.IsValid())
	{
		TOnlineResult<FAuthGetLocalOnlineUserByPlatformUserId> AuthGetResult = OnlineServicesInfoInternal->AuthInterface->GetLocalOnlineUserByPlatformUserId(MoveTemp(GetUserParams));
 
		if (AuthGetResult.IsOk())
		{
			FAuthGetLocalOnlineUserByPlatformUserId::Result& LocalOnlineUser = AuthGetResult.GetOkValue();
			TSharedRef<FAccountInfo> UserAccountInfo = LocalOnlineUser.AccountInfo;
			FAccountInfo UserAccountInfoContent = *UserAccountInfo;
			if (!OnlineUserInfos.Contains(UserAccountInfoContent.PlatformUserId))
			{
				UOnlineUserInfo* NewUser = CreateAndRegisterUserInfo(UserAccountInfoContent.AccountId.GetHandle(), PlatformUserId, UserAccountInfoContent.AccountId, UserAccountInfoContent.AccountId.GetOnlineServicesType());
 
				UE_LOG(LogOnlineSampleOnlineSubsystem, Log, TEXT("Local User Registered: %s"), *(NewUser->DebugInfoToString()));
			}
			else
			{
				UE_LOG(LogOnlineSampleOnlineSubsystem, Log, TEXT("Local User with platform user id %d already registered."), PlatformUserId.GetInternalId());
			}
		}
		else
		{
			FOnlineError ErrorResult = AuthGetResult.GetErrorValue();
			UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Get Local Online User Error: %s"), *ErrorResult.GetLogString());
		}
	}
	else
	{
		UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Auth Interface pointer invalid."));
	}
}

void UOnlineSampleOnlineSubsystem::K2_CreateSession(int32 MaxPlayer, bool IsLan, FPlatformUserId PlatformUserId)
{

	
	UE::Online::FCreateSession::Params SessionParams;
	SessionParams.bPresenceEnabled = true;
	SessionParams.bIsLANSession = IsLan;
	SessionParams.SessionSettings.NumMaxConnections = MaxPlayer;
	SessionParams.SessionSettings.SchemaName = TEXT("GameSession");
	SessionParams.SessionName = TEXT("MySessionName");
	//SessionParams.LocalAccountId = 
	//SessionParams.LocalAccountId = 0;

	CreateSession(SessionParams, PlatformUserId);
}

void UOnlineSampleOnlineSubsystem::CreateSession(UE::Online::FCreateSession::Params SessionParams, FPlatformUserId PlatformUserId)
{
	using namespace UE::Online;

	ISessionsPtr SessionInterface = OnlineServicesInfoInternal->SessionsInterface;

	 if(OnlineUserInfos.Contains(PlatformUserId))
	 {

		TObjectPtr<const UOnlineUserInfo> OnlineUser = *OnlineUserInfos.Find(PlatformUserId);
		FAccountId LocalAccountId = OnlineUser->AccountId;
		SessionParams.LocalAccountId = LocalAccountId;
		
		if(SessionInterface.IsValid())
		{
			SessionInterface->CreateSession(MoveTemp(SessionParams)).OnComplete(this, &ThisClass::HandleCreateSession);
		}
		else
		{
			UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Sessions Interface pointer invalid."));
		}
	 }else
	 {
	 	UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Could not find user with Platform User Id: %d"), PlatformUserId.GetInternalId());
	 }
}

void UOnlineSampleOnlineSubsystem::FindSessions(UE::Online::FFindSessions::Params SessionFindParams)
{
	using namespace UE::Online;

	ISessionsPtr SessionInterface = OnlineServicesInfoInternal->SessionsInterface;

	if(SessionInterface.IsValid())
	{
		
		UE_LOG(LogTemp, Warning, TEXT("Start FindSessions With Parameter - MaxResults : %d, LocalAccountId : %d"),
			SessionFindParams.MaxResults, SessionFindParams.LocalAccountId.GetHandle());
		
		
		SessionInterface->FindSessions(MoveTemp(SessionFindParams)).OnComplete([this, SessionInterface](const UE::Online::TOnlineResult<FFindSessions> &FindSessionsResult)
		{
			if(FindSessionsResult.IsOk())
			{
				TArray<FOnlineSessionId> SessionIds =  FindSessionsResult.GetOkValue().FoundSessionIds;
				for(FOnlineSessionId SessionId : SessionIds)
				{
					FGetSessionById::Params GetSessionParams;
					GetSessionParams.SessionId = SessionId;
					TOnlineResult<FGetSessionById> GetSessionByIdResult =  SessionInterface->GetSessionById(MoveTemp(GetSessionParams));
					if(GetSessionByIdResult.IsOk())
					{
						FBlueprintSessionInfo SessionInfo;
						GetSessionByIdResult.GetOkValue().Session->DumpState();
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Find Sessions Failed : %s"), *FindSessionsResult.GetErrorValue().GetLogString())	
			}
			
		});

		
	}
}

void UOnlineSampleOnlineSubsystem::K2_CreateLobby(ULocalPlayer* LocalPlayer, const FCreateLobbyRequest& Request)
{
	//
	check(!Request.LevelToTravel.IsNull());//
	
	CreateLobby(LocalPlayer, Request);
}

void UOnlineSampleOnlineSubsystem::CreateLobby(ULocalPlayer* LocalPlayer,const FCreateLobbyRequest& Request)
{
	using namespace UE::Online;
	
	if(!IsLoggedIn(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Create Lobby Failed : Not Logged In"));
		return;
	}
	
	
	if(UE::Online::ILobbiesPtr LobbiesInterface = OnlineServicesInfoInternal->LobbiesInterface)
	{
		UE::Online::FCreateLobby::Params CreateLobbyParams;

		//const FName SessionName(NAME_GameSession); // 일단 일반 사용자 플러그인에서 긁어온거. 흠..
		
		CreateLobbyParams.SchemaId = FSchemaId(TEXT("GameLobby"));
		CreateLobbyParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
		//CreateLobbyParams.LocalName = SessionName;
		CreateLobbyParams.LocalName = Request.LobbyName;//
		CreateLobbyParams.bPresenceEnabled = true;
		CreateLobbyParams.JoinPolicy = UE::Online::ELobbyJoinPolicy::PublicAdvertised;
		CreateLobbyParams.Attributes.Emplace(FName(TEXT("GAMEMODE")), FString(TEXT("GAMEMODE1")));
		CreateLobbyParams.Attributes.Emplace(FName(TEXT("MAPNAME")), FString(TEXT("MAP1")));
		//CreateLobbyParams.Attributes.Emplace(FName(TEXT("MATCHTIMEOUT")), 120.0f);
		//CreateLobbyParams.Attributes.Emplace(FName(TEXT("SESSIONTEMPLATENAME")), FString(TEXT("GameSession")));
		//CreateLobbyParams.Attributes.Emplace(FName(TEXT("OSSv2")), true);
		CreateLobbyParams.Attributes.Emplace(FName(TEXT("PRESENCESEARCH")), true);
		CreateLobbyParams.Attributes.Emplace(FName(TEXT("GAMEMODE")), FString(TEXT("GameSession")));
		CreateLobbyParams.Attributes.Emplace(FName(TEXT("MATCHSTATE")), FString(TEXT("Waiting")));
		
		CreateLobbyParams.MaxMembers = Request.MaxPlayers;

		//유저 아트리뷰트
		CreateLobbyParams.UserAttributes.Emplace(FName(TEXT("GAMEMODE")), FString(TEXT("GameSession")));
		CreateLobbyParams.UserAttributes.Emplace(FName(TEXT("MATCHSTATE")), FString(TEXT("Waiting")));
		
		LobbiesInterface->CreateLobby(MoveTemp(CreateLobbyParams)).OnComplete(this, &ThisClass::HandleCreateLobby);
		
	}
	
}

void UOnlineSampleOnlineSubsystem::K2_FindLobbies(ULocalPlayer* LocalPlayer)
{
	UE::Online::FFindLobbies::Params FindLobbyParams;
	FindLobbies(LocalPlayer, FindLobbyParams);
}

void UOnlineSampleOnlineSubsystem::FindLobbiesByUser(ULocalPlayer* LocalPlayer, const FBlueprintFriendInfo& FriendInfo)
{
	using namespace UE::Online;
	FFindLobbies::Params FindLobbyParams;
	FindLobbyParams.TargetUser = FriendInfo.Friend->FriendId;
	FindLobbies(LocalPlayer,FindLobbyParams);
}

void UOnlineSampleOnlineSubsystem::FindLobbies(ULocalPlayer* LocalPlayer, UE::Online::FFindLobbies::Params FindLobbyParams)
{
	using namespace UE::Online;
	
	if(!IsLoggedIn(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Local Player is not logged in"));
		return;
	}

	if(ILobbiesPtr LobbiesInterface = OnlineServicesInfoInternal->LobbiesInterface)
	{
				
		FindLobbyParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
		FindLobbyParams.MaxResults = 10;
		//FindLobbyParams.Filters.Emplace(FFindLobbySearchFilter{FName(TEXT("OSSv2")), ESchemaAttributeComparisonOp::Equals, true});
		
		FindLobbyParams.Filters.Emplace(FFindLobbySearchFilter{ FName(TEXT("PRESENCESEARCH")), ESchemaAttributeComparisonOp::Equals, true });
		
		LobbiesInterface->FindLobbies(MoveTemp(FindLobbyParams)).OnComplete(this, &ThisClass::HandleFindLobbies);
	}

	
}

void UOnlineSampleOnlineSubsystem::K2_JoinLobby(ULocalPlayer* LocalPlayer, const FBlueprintLobbyInfo& LobbyInfoToJoin)
{
	JoinLobby(LocalPlayer, LobbyInfoToJoin);
}

void UOnlineSampleOnlineSubsystem::JoinFriendLobby(ULocalPlayer* LocalPlayer, const FBlueprintFriendInfo& FriendInfo)
{
	
}

void UOnlineSampleOnlineSubsystem::JoinLobby(ULocalPlayer* LocalPlayer, const FBlueprintLobbyInfo& LobbyInfoToJoin)
{
	using namespace UE::Online;
	check(LocalPlayer);

	if(!IsLoggedIn(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Local Player is not logged in"));
		return;
	}
	
	if(ILobbiesPtr LobbiesInterface = OnlineServicesInfoInternal->LobbiesInterface)
	{
		const FName SessionName(NAME_GameSession);//
		
		FJoinLobby::Params JoinLobbyParams;
		const FLobby* LobbyToJoin = LobbyInfoToJoin.Lobby.Get();
		
		JoinLobbyParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
		JoinLobbyParams.LobbyId = LobbyToJoin->LobbyId;
		JoinLobbyParams.bPresenceEnabled = true;
		//JoinLobbyParams.LocalName = LobbyToJoin->LocalName;
		JoinLobbyParams.LocalName = SessionName;//
		LobbiesInterface->JoinLobby(MoveTemp(JoinLobbyParams)).OnComplete(this, &ThisClass::HandleJoinLobby);	
		
	}
}

void UOnlineSampleOnlineSubsystem::K2_LeaveLobby(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo)
{
	LeaveLobby(LocalPlayer, LobbyInfo.Lobby->LobbyId);
}

void UOnlineSampleOnlineSubsystem::LeaveLobby(ULocalPlayer* LocalPlayer, UE::Online::FLobbyId LobbyId)
{
	using namespace UE::Online;
	check(LocalPlayer);

	if(!IsLoggedIn(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Local Player is not logged in"));
		return;
	}
	
	LeaveLobby(LocalPlayer->GetPlatformUserId(), LobbyId);
}

void UOnlineSampleOnlineSubsystem::LeaveLobby(FPlatformUserId PlatformUserId, UE::Online::FLobbyId LobbyId)
{
	using namespace UE::Online;
	
	if(ILobbiesPtr LobbiesInterface = OnlineServicesInfoInternal->LobbiesInterface)
	{

		FLeaveLobby::Params LeaveLobbyParams;
		LeaveLobbyParams.LobbyId = LobbyId;
		LeaveLobbyParams.LocalAccountId = GetOnlineUserInfo(PlatformUserId)->AccountId;
		
		LobbiesInterface->LeaveLobby(MoveTemp(LeaveLobbyParams)).OnComplete([this](TOnlineResult<FLeaveLobby> LeaveLobbyResult)
		{
			if(LeaveLobbyResult.IsOk())
			{
				LocalPlayerLobbyMemberInfo = FBlueprintLobbyMemberInfo();
				UE_LOG(LogTemp, Display, TEXT("Leave Lobby Complete"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Leave Lobby Failed : %s"), *LeaveLobbyResult.GetErrorValue().GetLogString());
			}
		});

		NotifyLobbyUpdated();
	}
}

void UOnlineSampleOnlineSubsystem::InitFriendsInfo(ULocalPlayer* LocalPlayer)
{
	using namespace UE::Online;
	
	//델리게이트 바인드 순서가 문제가 될 수도 있으려나? (바인드 한 다음에 이번 GetFriends 호출이 아닌 저번 호출 결과때문에 델리게이트가 실행된다거나.)
	FDelegateHandle FuncHandle = OnGetFriendsCompleteEvent.AddLambda([this, LocalPlayer, &FuncHandle](bool bSucceed)
	{
		if(bSucceed)
		{
			for(auto& Tuple: FoundFriends)
			{
				const FBlueprintFriendInfo FriendInfo = Tuple.Value;
				QueryPresence(LocalPlayer, FriendInfo.Friend->FriendId, true);	
			}
			
		}

		//지울 필요가 있나?
		OnGetFriendsCompleteEvent.Remove(FuncHandle);
	});
	
	GetFriends(LocalPlayer);

	
}

void UOnlineSampleOnlineSubsystem::StartGameFromLobby(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo)
{
	using namespace UE::Online;
	check(LocalPlayer);

	if(LobbyInfo.Lobby.Get()->OwnerAccountId != GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId)
	{
		UE_LOG(LogTemp, Error, TEXT("This Player is NOT Lobby Owner, Only Owner can start game."));
		return;
	}

	//
	FString LevelPath = MyMap.ToSoftObjectPath().GetLongPackageName() + "?listen";
	UE_LOG(LogTemp, Display, TEXT("LevelPath: %s"), *LevelPath);
	if(MyMap.IsValid())
	{
		GetWorld()->ServerTravel(LevelPath);
		AdjustLobbyAfterStart(LocalPlayer, LobbyInfo);
	}
	else if(MyMap.IsPending())
	{
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		Streamable.RequestAsyncLoad(MyMap.ToSoftObjectPath(), FStreamableDelegate::CreateLambda([this, LevelPath, LocalPlayer, LobbyInfo]()
		{
			GetWorld()->ServerTravel(LevelPath);
			AdjustLobbyAfterStart(LocalPlayer, LobbyInfo);
		}));
	}
	//
}

void UOnlineSampleOnlineSubsystem::TravelToLobby(ULocalPlayer* LocalPlayer, FBlueprintLobbyInfo LobbyInfo)
{
	using namespace UE::Online;

	if(IOnlineServicesPtr OnlineServices =  OnlineServicesInfoInternal->OnlineServices)
	{
		FGetResolvedConnectString::Params Params;
		Params.LobbyId = LobbyInfo.Lobby->LobbyId;
		Params.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;

		TOnlineResult<FGetResolvedConnectString> Result =  OnlineServices->GetResolvedConnectString(MoveTemp(Params));

		if(Result.IsOk())
		{
			UE_LOG(LogTemp, Warning, TEXT("ResolvedConectString URL for Client Travel : %s |And| %s"), *Result.GetOkValue().ResolvedConnectString, *MyMap.ToSoftObjectPath().GetLongPackageName());
			
			LocalPlayer->PlayerController->ClientTravel(Result.GetOkValue().ResolvedConnectString, TRAVEL_Absolute);
		}else
		{
			UE_LOG(LogTemp, Error, TEXT("Get Resolved Connect String Failed : %s"), *Result.GetErrorValue().GetLogString());
		}
		
		
	}
}

void UOnlineSampleOnlineSubsystem::GetFriends(ULocalPlayer* LocalPlayer)
{
	using namespace UE::Online;
	check(LocalPlayer);

	if(!IsLoggedIn(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Local Player is not logged in"));
		return;
	}

	
	if(ISocialPtr SocialPtr = OnlineServicesInfoInternal->SocialInterface)
	{
		FQueryFriends::Params QueryFriendsParam;
		QueryFriendsParam.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
		SocialPtr->QueryFriends(MoveTemp(QueryFriendsParam)).OnComplete([this, LocalPlayer, SocialPtr](TOnlineResult<FQueryFriends> QueryFriendsResult)
		{
			if(QueryFriendsResult.IsOk())
			{
				FGetFriends::Params GetFriendsParams;
				GetFriendsParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
				TOnlineResult<FGetFriends> GetFriendsResult = SocialPtr->GetFriends(MoveTemp(GetFriendsParams));
				HandleGetFriends(GetFriendsResult);
				
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Get Friends Failed : %s"), *QueryFriendsResult.GetErrorValue().GetLogString());
			}
			
		});
	}
}



void UOnlineSampleOnlineSubsystem::QueryPresence(ULocalPlayer* LocalPlayer, UE::Online::FAccountId TargetId, bool bListenToChanges)
{
	using namespace UE::Online;
	check(LocalPlayer);

	if(!IsLoggedIn(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Local Player is not logged in"));
		return;
	}

	
	if(IPresencePtr PresenceInterface = OnlineServicesInfoInternal->PresenceInterface)
	{
		FQueryPresence::Params QueryPresenceParams;
		QueryPresenceParams.bListenToChanges = bListenToChanges;
		QueryPresenceParams.TargetAccountId = TargetId;
		QueryPresenceParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
		PresenceInterface->QueryPresence(MoveTemp(QueryPresenceParams)).OnComplete([this, LocalPlayer, PresenceInterface](TOnlineResult<FQueryPresence> QueryPresenceResult)
		{
			if(QueryPresenceResult.IsOk())
			{
				FUserPresence ResultPresence = QueryPresenceResult.GetOkValue().Presence.Get();

				//
				UE_LOG(LogTemp, Warning, TEXT("Query User Presence Id: %d"), ResultPresence.AccountId.GetHandle());
				UE_LOG(LogTemp, Warning, TEXT("Query User Presence Status: %d"), ResultPresence.Status);
				UE_LOG(LogTemp, Warning, TEXT("Query User Presence StatusString: %s"), *ResultPresence.StatusString);
				//
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Get Friends Failed : %s"), *QueryPresenceResult.GetErrorValue().GetLogString());
			}
			
		});

	}
}

void UOnlineSampleOnlineSubsystem::GetUserInfo(ULocalPlayer* LocalPlayer, TArray<UE::Online::FAccountId> TargetUsers)
{
	using namespace UE::Online;
	check(LocalPlayer);

	if(!IsLoggedIn(LocalPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Local Player is not logged in"));
		return;
	}

	if(IUserInfoPtr UserInfoInterface = OnlineServicesInfoInternal->UserInfoInterface)
	{
		FoundUsers.Empty();
		
		FQueryUserInfo::Params QueryUserInfoParam;
		QueryUserInfoParam.AccountIds.Insert(TargetUsers, 0);
		QueryUserInfoParam.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
		UserInfoInterface->QueryUserInfo(MoveTemp(QueryUserInfoParam)).OnComplete([this, LocalPlayer, UserInfoInterface, TargetUsers]
			(TOnlineResult<FQueryUserInfo> QueryUserInfoResult)
		{
			if(QueryUserInfoResult.IsOk())
			{
				for(const FAccountId TargetId : TargetUsers)
				{
					FGetUserInfo::Params GetUserInfoParams;
					GetUserInfoParams.AccountId = TargetId;
					GetUserInfoParams.LocalAccountId = GetOnlineUserInfo(LocalPlayer->GetPlatformUserId())->AccountId;
					TOnlineResult<FGetUserInfo> GetUserInfoResult = UserInfoInterface->GetUserInfo(MoveTemp(GetUserInfoParams));
					HandleGetUserInfo(GetUserInfoResult);
				}
				
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Query User Info Failed : %s"), *QueryUserInfoResult.GetErrorValue().GetLogString());
			}
			
		});
		
		
		
	}
}

void UOnlineSampleOnlineSubsystem::Login(FPlatformUserId PlatformUserId)																														
{
	using namespace UE::Online;
	
	if(const UE::Online::IAuthPtr AuthInterface = OnlineServicesInfoInternal->AuthInterface)
	{
		FAuthGetLocalOnlineUserByPlatformUserId::Params GetUserParams;
		GetUserParams.PlatformUserId = PlatformUserId;
		
		TOnlineResult<FAuthGetLocalOnlineUserByPlatformUserId> LocalUserSearchResult = AuthInterface->GetLocalOnlineUserByPlatformUserId(MoveTemp(GetUserParams));
		// 일단 로컬 온라인 유저가 검색이 된다는 것은 이미 로그인 되어 있는 것, 이라고 가정.
		// TODO : 위 가설 검증.
		if(LocalUserSearchResult.IsOk())
		{
			if (!OnlineUserInfos.Contains(PlatformUserId))
			{
				const TSharedRef<FAccountInfo> AccountInfo =  LocalUserSearchResult.GetOkValue().AccountInfo;
				UOnlineUserInfo* NewUser = CreateAndRegisterUserInfo(AccountInfo->AccountId.GetHandle(), PlatformUserId, AccountInfo->AccountId, AccountInfo->AccountId.GetOnlineServicesType());
 
				UE_LOG(LogOnlineSampleOnlineSubsystem, Warning, TEXT("Local User Registered: %s"), *(NewUser->DebugInfoToString()));
			}

			OnLoginCompleteEvent.Broadcast(LocalUserSearchResult.IsOk());
			K2_OnLoginCompleteEvent.Broadcast(LocalUserSearchResult.IsOk());
			
			return;
		}
		
		
		FAuthLogin::Params LoginParams;
		//LoginParams.CredentialsId = FString("localhost:8081");
		LoginParams.PlatformUserId = PlatformUserId;
		
		//LoginParams.CredentialsToken.Emplace<FString>("TestCredentialName");
		LoginParams.CredentialsType = LoginCredentialsType::AccountPortal;
		//LoginParams.CredentialsType = LoginCredentialsType::Developer;
		//LoginParams.
		
		OnlineServicesInfoInternal->AuthInterface->Login(MoveTemp(LoginParams)).OnComplete([this, PlatformUserId](const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result)
	{
		if(Result.IsOk()) 
		{
			const TSharedRef<UE::Online::FAccountInfo> AccountInfo = Result.GetOkValue().AccountInfo;
			
			if (!OnlineUserInfos.Contains(AccountInfo->PlatformUserId))
			{
				TObjectPtr<UOnlineUserInfo> NewUser = CreateAndRegisterUserInfo(AccountInfo->AccountId.GetHandle(), PlatformUserId, AccountInfo->AccountId, AccountInfo->AccountId.GetOnlineServicesType());
				
				UE_LOG(LogOnlineSampleOnlineSubsystem, Warning, TEXT("Local User Registered: %s"), *(NewUser->DebugInfoToString()));
			}
			else
			{
				UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Local User with platform user id %d already registered."), PlatformUserId.GetInternalId());
			}
			
		}
		else
		{
			FOnlineError Error = Result.GetErrorValue();
			// 이제 오류를 처리할 수 있습니다.
			UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Login Error: %s"), *Error.GetLogString());
		}

			OnLoginCompleteEvent.Broadcast(Result.IsOk());
			K2_OnLoginCompleteEvent.Broadcast(Result.IsOk());
	});
	}
	
}

void UOnlineSampleOnlineSubsystem::Logout()
{
	using namespace UE::Online;
	
	if(IAuthPtr AuthInterface = OnlineServicesInfoInternal->AuthInterface)
	{
		
		for(auto& Tuple : OnlineUserInfos)
		{
			
			const UOnlineUserInfo* UserInfo = Tuple.Value;

			//입장한 로비가 있다면 떠나기.
			if(JoinedLobby.Lobby.IsValid())
			{
				LeaveLobby(UserInfo->PlatformUserId, JoinedLobby.Lobby.Get()->LobbyId);
			}
			
			FAuthLogout::Params LogoutParams;
			LogoutParams.LocalAccountId =UserInfo->AccountId;
			AuthInterface->Logout(MoveTemp(LogoutParams)).OnComplete([this,UserInfo](TOnlineResult<FAuthLogout> LogoutResult)
			{
				if(LogoutResult.IsOk()){
					OnlineUserInfos.Remove(UserInfo->PlatformUserId);
					UE_LOG(LogTemp, Display, TEXT("Logout Complete "));
				}
				else {
					UE_LOG(LogTemp, Error, TEXT("Logout Failed : %s"), *LogoutResult.GetErrorValue().GetLogString());
				}
			});
		}
	}
}

bool UOnlineSampleOnlineSubsystem::IsLoggedIn(ULocalPlayer* LocalPlayer)
{
	if(OnlineUserInfos.Contains(LocalPlayer->GetPlatformUserId()))
	{
		const UOnlineUserInfo* Info = *OnlineUserInfos.Find(LocalPlayer->GetPlatformUserId());

		if(UE::Online::IAuthPtr AuthInterface = OnlineServicesInfoInternal->AuthInterface)
		{
			return AuthInterface->IsLoggedIn(Info->AccountId);
		}
	}
	
	return false;
}

void UOnlineSampleOnlineSubsystem::K2_FindSessions(APlayerController* PlayerController, int32 MaxResults, bool bUseLan)
{
	using namespace UE::Online;
	check(PlayerController);

	if(const UOnlineUserInfo* OnlineUserInfo =  GetOnlineUserInfo(PlayerController->GetPlatformUserId()))
	{
		if(UE::Online::IAuthPtr AuthInterface = OnlineServicesInfoInternal->AuthInterface)
		{
			if(!AuthInterface->IsLoggedIn(OnlineUserInfo->AccountId))
			{
				return;
			}
		}
		else
		{
			return;
		}
		
		FFindSessions::Params FindSessionsParams;
		FindSessionsParams.MaxResults = MaxResults;
		FindSessionsParams.bFindLANSessions = bUseLan;
		FindSessionsParams.LocalAccountId = OnlineUserInfo->AccountId;
		//FindSessionsParams.Filters

		FindSessions(FindSessionsParams);
	}
}

/// <summary>
/// 주어진 정보로 UOnlineUserInfo 오브젝트를 생성합니다.
/// </summary>
/// <param name="LocalUserIndex"></param>
/// <param name="PlatformUserId"></param>
/// <param name="AccountId"></param>
/// <param name="Services">사용자가 등록되는 온라인 서비스입니다.</param>
/// <returns>NewUser에 대한 오브젝트 포인터입니다</returns>
TObjectPtr<UOnlineUserInfo> UOnlineSampleOnlineSubsystem::CreateOnlineUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, UE::Online::FAccountId AccountId, UE::Online::EOnlineServices Services)
{
	TObjectPtr<UOnlineUserInfo> NewUser = NewObject<UOnlineUserInfo>(this);
	NewUser->LocalUserIndex = LocalUserIndex;
	NewUser->PlatformUserId = PlatformUserId;
	NewUser->AccountId = AccountId;
	NewUser->Services = Services;
	return NewUser;
}
 
/// <summary>
/// CreateOnlineUserInfo를 호출하여 UOnlineUserInfo 오브젝트를 생성한 다음
///		로컬 레지스트리 OnlineUserInfos에 사용자를 등록합니다
/// </summary>
/// <param name="LocalUserIndex"></param>
/// <param name="PlatformUserId"></param>
/// <param name="AccountId"></param>
/// <param name="Services">사용자가 등록되는 온라인 서비스입니다.</param>
/// <returns>NewUser에 대한 오브젝트 포인터입니다</returns>
TObjectPtr<UOnlineUserInfo> UOnlineSampleOnlineSubsystem::CreateAndRegisterUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, UE::Online::FAccountId AccountId, UE::Online::EOnlineServices Services)
{
	TObjectPtr<UOnlineUserInfo> NewUser = CreateOnlineUserInfo(LocalUserIndex, PlatformUserId, AccountId, Services);
	OnlineUserInfos.Add(PlatformUserId.GetInternalId(), NewUser);
	return NewUser;
}
 
/// <summary>
/// 제공된 플랫폼 사용자 ID에 대한 UOnlineUserInfo를 얻습니다.
/// </summary>
/// <param name="PlatformUserId">가져올 사용자 ID입니다</param>
/// <returns>OnlineUser에 대한 오브젝트 포인터입니다</returns>
TObjectPtr<const UOnlineUserInfo> UOnlineSampleOnlineSubsystem::GetOnlineUserInfo(FPlatformUserId PlatformUserId)
{
	TObjectPtr<const UOnlineUserInfo> OnlineUser;
	if (OnlineUserInfos.Contains(PlatformUserId))
	{
		OnlineUser = *OnlineUserInfos.Find(PlatformUserId);
	}
	else
	{
		UE_LOG(LogOnlineSampleOnlineSubsystem, Error, TEXT("Could not find user with Platform User Id: %d"), PlatformUserId.GetInternalId());
		OnlineUser = nullptr;
	}
	return OnlineUser;
}
 
/// <summary>
/// UOnlineUserInfo 오브젝트에 대한 생성자입니다
/// </summary>
UOnlineUserInfo::UOnlineUserInfo()
{
 
}
 
/// <summary>
/// UOnlineUserInfo에 대한 디버그 스트링을 반환합니다
/// </summary>
/// <returns>UOnlineUserInfo의 스트링 표현입니다</returns>
const FString UOnlineUserInfo::DebugInfoToString()
{
	int32 UserIndex = this->LocalUserIndex;
	int32 PlatformId = this->PlatformUserId;
	uint32 AccId = this->AccountId.GetHandle();
	TArray<FStringFormatArg> FormatArgs;
	FormatArgs.Add(FStringFormatArg(UserIndex));
	FormatArgs.Add(FStringFormatArg(PlatformId));
	FormatArgs.Add(FStringFormatArg(AccId));
	return FString::Format(TEXT("LocalUserNumber: {0}, PlatformUserId: {1}, AccountId: {2}"), FormatArgs);
}