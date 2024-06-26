#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/ProfilePage.hpp>

using namespace geode::prelude;

std::map<std::string, ccColor3B> colors_map = {
	{"online", cc3bFromHexString("#00FF2A").value()},
	{"idle", cc3bFromHexString("#FFEE00").value()},
	{"dnd", cc3bFromHexString("#FF0000").value()},
	{"offline", cc3bFromHexString("#BEBEBE").value()},
	{"null", cc3bFromHexString("#BEBEBE").value()}
};


bool safeToUpdate = false;


class $modify(StatusProfilePage, ProfilePage) {
	struct Fields {
		bool status_own_profile;
		bool status_loaded;
		std::string status_string;
	};
	static void onModify(auto & self)
	{
		(void) self.setHookPriority("ProfilePage::loadPageFromUserInfo", -1000);
	}
	void roundRobinStatus() {
		if (m_fields->status_string == "null") {
			m_fields->status_string = "online";
			return;
		}

		if (m_fields->status_string == "online") {
			m_fields->status_string = "idle";
			return;
		}

		if (m_fields->status_string == "idle") {
			m_fields->status_string = "dnd";
			return;
		}

		if (m_fields->status_string == "dnd") {
			m_fields->status_string = "offline";
			return;
		}

		if (m_fields->status_string == "offline") {
			m_fields->status_string = "online";
			return;
		}
	}

	void onStatusClick(CCObject* sender) {
		if (!m_fields->status_loaded) return;

		if (m_accountID != GJAccountManager::get()->m_accountID) {
			const char* user_status = m_fields->status_string == "null" ? "This user hasn't set a status." : m_fields->status_string.c_str();

			FLAlertLayer::create(
				std::string(std::string(m_usernameLabel->getString()) + std::string("'s status")).c_str(),
				user_status,
				"OK"
			)->show();
			return;
		}

		// log::debug("Status string: {}", m_fields->status_string);

		roundRobinStatus();

		// log::debug("Updated status string: {}", m_fields->status_string);

		updateStatusDot(m_fields->status_string);
		web::AsyncWebRequest()
			.fetch(
				"https://gdstatus.7m.pl/updateStatus.php?accountID=" 
				+ std::to_string(m_accountID) 
				+ "&status=" 
				+ m_fields->status_string
			)
			.text()
			.then([&](std::string const& result) {
				log::info("Updated status");
				Notification::create(
					"Updated status to " + m_fields->status_string,
					CCSprite::create("GJ_completesIcon_001.png")
				)->show();
			})
			.expect([](std::string const& error) {
				if (error.find("DOCTYPE HTML") != std::string::npos) {
					Notification::create(
						"Error updating status: " + error,
						CCSprite::create("GJ_deleteIcon_001.png")
					)->show();
				} else {
					Notification::create(
						"Error updating status. Please report this error ASAP." + error,
						CCSprite::create("GJ_reportBtn_001.png")
					)->show();
				}
			});
	}

	void addStatusDot(bool own_profile) {
		m_fields->status_own_profile = own_profile;

		auto status_dot = CCMenuItemLabel::create(
			CCLabelBMFont::create(".", "bigFont.fnt"), 
			this, menu_selector(StatusProfilePage::onStatusClick)
		);

		auto status_menu = CCMenu::create();

		status_menu->setID("status-menu"_spr);
		status_menu->setZOrder(300);
		status_menu->addChild(status_dot);
		m_mainLayer->addChild(status_menu);

		status_dot->setID("status_dot"_spr);

		if (Loader::get()->isModLoaded("itzkiba.better_progression")) {
			if (auto theBG = m_mainLayer->getChildByID("icon-background")) {
				status_menu->setPosition({0, 0});
				status_dot->setPosition({theBG->getPositionX(), 210.5f});
				status_dot->setContentSize({10.f, 10.f}); // a bit easier to click on when betterprogression is installed
			}
		} else {
			status_menu->setPosition({(CCDirector::get()->getWinSize().width / 2.f) - 170.f, 220.f});
			status_dot->setContentSize({8.750f, 12.5f});
		}
		status_dot->setScale(1.825f);

		status_dot->setColor(cc3bFromHexString("#BEBEBE").value());
	}

	void updateStatusDot(std::string status) {
		if (!this) return;

		m_fields->status_string = status;

		auto status_dot = static_cast<CCMenuItemLabel*>(getChildByIDRecursive("status_dot"_spr));

		if (!status_dot) return;

		status_dot->setColor(colors_map[status]);
	}

	void setupStatus(int account_id, bool own_profile) {
		addStatusDot(own_profile);

		// log::info("Account ID: {}", account_id);

		web::AsyncWebRequest()
			.fetch("https://gdstatus.7m.pl/getStatus.php?accountID=" + std::to_string(account_id))
			.text()
			.then([&](std::string const& result) {
				if (!safeToUpdate) return;

				m_fields->status_loaded = true;
				updateStatusDot(result);
			})
			.expect([](std::string const& error) {
				if (error.find("DOCTYPE HTML") != std::string::npos) {
					Notification::create(
						"Error updating status: " + error,
						CCSprite::create("GJ_deleteIcon_001.png")
					)->show();
				} else {
					Notification::create(
						"Error updating status. Please report this error ASAP." + error,
						CCSprite::create("GJ_reportBtn_001.png")
					)->show();
				}
			});
	}


	void onClose(CCObject* sender) {
		safeToUpdate = false;

		ProfilePage::onClose(sender);
	}


	bool init(int account_id, bool own_profile) {
		if (!ProfilePage::init(account_id, own_profile)) return false;

		safeToUpdate = true;

		m_fields->status_loaded = false;
		setupStatus(account_id, own_profile);

		return true;
	}
};