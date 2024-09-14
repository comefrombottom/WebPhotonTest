# include <Siv3D.hpp> // Siv3D v0.6.15
# include "Multiplayer_Photon.hpp"
# include "PHOTON_APP_ID.SECRET"

namespace EventCode {
	enum : uint8
	{
		//イベントコードは1から199までの範囲を使う
		sendWord = 1,
		sendResult = 2,
	};
}

class MyClient : public Multiplayer_Photon
{
public:
	MyClient()
	{
		init(std::string(SIV3D_OBFUSCATE(PHOTON_APP_ID)), U"1.0", Verbose::No, ConnectionProtocol::Wss);

		RegisterEventCallback(EventCode::sendWord, &MyClient::eventReceived_sendWord);
		RegisterEventCallback(EventCode::sendResult, &MyClient::eventReceived_sendResult);

	}


private:

	//イベントを受信したらそれに応じた処理を行う

	void eventReceived_sendWord([[maybe_unused]] LocalPlayerID playerID, const String& word)
	{
		ClearPrint();
		Print << U"受け取った言葉:{} "_fmt(word);
	}

	void eventReceived_sendResult([[maybe_unused]] LocalPlayerID playerID, const String& result)
	{
		Print << result;
	}

	void joinRoomEventAction(const LocalPlayer& newPlayer, [[maybe_unused]] const Array<LocalPlayerID>& playerIDs, bool isSelf) override
	{

	}
};

std::pair<String, String> randomHiraganaWordPair(size_t n, size_t change) {
	static Array<String> hiragana = { U"あ", U"い", U"う", U"え", U"お", U"か", U"き", U"く", U"け", U"こ", U"さ", U"し", U"す", U"せ", U"そ", U"た", U"ち", U"つ", U"て", U"と", U"な", U"に", U"ぬ", U"ね", U"の", U"は", U"ひ", U"ふ", U"へ", U"ほ", U"ま", U"み", U"む", U"め", U"も", U"や", U"ゆ", U"よ", U"ら", U"り", U"る", U"れ", U"ろ", U"わ", U"を", U"ん" };

	Array<size_t> indices;

	for (auto i : step(n))
	{
		indices.push_back(Random<size_t>(0, hiragana.size() - 1));
	}

	Array<size_t> indices2 = indices;
	Array<size_t> choiceIndex = step(n);
	choiceIndex.shuffle();
	for (auto i : step(change))
	{
		if (choiceIndex.size() <= i) break;
		indices[choiceIndex[i]] = Random<size_t>(0, hiragana.size() - 1);
	}

	String result;

	for (auto i : indices)
	{
		result += hiragana[i];
	}

	String result2;

	for (auto i : indices2)
	{
		result2 += hiragana[i];
	}

	return { result,result2 };
}

void Main()
{
	MyClient client;

	Font font(20);

	TextEditState playerName(U"player" + ToHex(RandomUint8()));

	Array<LocalPlayerID> wolfID;

	bool isGameStarted = false;

	String trueWord;
	String wolfWord;

	int32 wordSize = 5;
	int32 changeSize = 2;

	int32 wolfCount = 1;

	while (System::Update())
	{
		if (client.isActive())
		{
			client.update();
		}
		else {
			SimpleGUI::TextBox(playerName, Vec2(50, 50), 200);

			if (SimpleGUI::Button(U"Connect", Vec2(50, 100)))
			{
				client.connect(playerName.text, U"jp");
			}

			font(U"v0.3").draw(620, 550);
		}

		if (client.isInLobby())
		{
			client.joinRandomOrCreateRoom(U"");
		}

		if (client.isInRoom())
		{
			Scene::Rect().draw(Palette::Sienna);

			if (client.isHost()) {
				if (not isGameStarted) {
					{
						double wordSize_d = wordSize;
						if (SimpleGUI::Slider(U"word:{}"_fmt(wordSize), wordSize_d, 1, 10, Vec2(550, 100), 100)) {
							wordSize = static_cast<int32>(wordSize_d);
						}
					}
					{
						double changeSize_d = changeSize;
						if (SimpleGUI::Slider(U"change:{}"_fmt(changeSize), changeSize_d, 1, 10, Vec2(550, 150), 100)) {
							changeSize = static_cast<int32>(changeSize_d);
						}
					}
					{
						double wolfCount_d = wolfCount;
						if (SimpleGUI::Slider(U"wolf:{}"_fmt(wolfCount), wolfCount_d, 1, client.getPlayerCountInCurrentRoom(), Vec2(550, 200), 100)) {
							wolfCount = static_cast<int32>(wolfCount_d);
						}
					}


					if (SimpleGUI::Button(U"Start Game", Vec2(620, 30))) {
						isGameStarted = true;

						auto ws = randomHiraganaWordPair(5, 2);
						trueWord = ws.first;
						wolfWord = ws.second;

						auto playerIDs = client.getLocalPlayerIDs();
						wolfID = playerIDs.choice(wolfCount);

						for (auto id : playerIDs) {
							if (wolfID.contains(id)) {
								client.sendEvent({ EventCode::sendWord, {id} }, wolfWord);
							}
							else {
								client.sendEvent({ EventCode::sendWord, {id} }, trueWord);
							}
						}

					}
				}
				else {
					if (SimpleGUI::Button(U"End Game", Vec2(620, 30))) {
						isGameStarted = false;
						String wolfNames;

						for (auto id : wolfID) {
							wolfNames += U"\n";
							wolfNames += client.getLocalPlayer(id).userName;
						}
						client.sendEvent({ EventCode::sendResult, ReceiverOption::All }, U"result:\n人狼は:{}\n村の言葉:{}\n人狼の言葉:{}"_fmt(wolfNames, trueWord, wolfWord));
					}

				}


			}

			//プレイヤー名

			font(U"プレイヤーリスト").draw(300, 10);
			for (auto [i, player] : Indexed(client.getLocalPlayers())) {
				if (client.getLocalPlayerID() == player.localID) {
					font(player.userName + U"(自分)").draw(300, 40 + i * 30);
				}
				else {
					font(player.userName).draw(300, 40 + i * 30);
				}
			}

		}

		if (not client.isDisconnected() and not client.isInRoom()) {
			//ローディング画面
			size_t t = static_cast<size_t>(Floor(fmod(Scene::Time() / 0.1, 8)));
			for (size_t i : step(8)) {
				Vec2 n = Circular(1, i * Math::TwoPi / 8);
				Line(Scene::Center() + n * 10, Arg::direction(n * 10)).draw(LineStyle::RoundCap, 4, t == i ? ColorF(1, 0.9) : ColorF(1, 0.5));
			}
		}

		if (client.isActive()) {
			if (SimpleGUI::Button(U"Disconnect", Vec2(620, 550))) {
				client.disconnect();
			}
		}
	}
}
