include $(TOPDIR)/rules.mk

PKG_NAME:=openvpn_monitoring
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/openvpn_monitoring
  TITLE:=OpenVPN Monitoring
  DEPENDS:=+libubus-lua +libubox-lua +libuci-lua +luci-lib-nixio +liblua +openvpn
endef

define Package/openvpn_monitoring/description
	Provides UBUS objects with methods to list connected clients with functionality to disconnect them.
endef

define Build/Compile
endef

define Package/openvpn_monitoring/install
	$(INSTALL_DIR) $(1)/usr/lib/lua/openvpn_monitoring
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/usr/lib/lua/openvpn_monitoring/*.lua $(1)/usr/lib/lua/openvpn_monitoring/
	$(INSTALL_BIN) ./files/openvpn_monitoring.init $(1)/etc/init.d/openvpn_monitoring
endef

$(eval $(call BuildPackage,openvpn_monitoring))
