/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/util/Format.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/form/field/UserComboBox.js")
// require("js/omv/window/Execute.js")

/**
 * @class OMV.module.admin.system.cron.Job
 * @derived OMV.workspace.window.Form
 */
Ext.define("OMV.module.admin.system.cron.Job", {
	extend: "OMV.workspace.window.Form",
	requires: [
		"OMV.form.field.UserComboBox",
		"OMV.workspace.window.plugin.ConfigObject"
	],

	rpcService: "Cron",
	rpcGetMethod: "get",
	rpcSetMethod: "set",
	plugins: [{
		ptype: "configobject"
	}],
	height: 415,

	/**
	 * The class constructor.
	 * @fn constructor
	 * @param uuid The UUID of the database/configuration object. Required.
	 */

	getFormItems: function() {
		return [{
			xtype: "checkbox",
			name: "enable",
			fieldLabel: _("Enable"),
			checked: true
		},{
			xtype: "compositefield",
			fieldLabel: _("Minute"),
			combineErrors: false,
			items: [{
				xtype: "combo",
				name: "minute",
				queryMode: "local",
				store: Array.range(0, 59, 1, true).insert(0, "*"),
				allowBlank: false,
				editable: false,
				triggerAction: "all",
				value: String(new Date().getMinutes()),
				flex: 1
			},{
				xtype: "checkbox",
				name: "everynminute",
				fieldLabel: "",
				checked: false,
				boxLabel: _("Every N minute"),
				width: 140
			}]
		},{
			xtype: "compositefield",
			fieldLabel: _("Hour"),
			combineErrors: false,
			items: [{
				xtype: "combo",
				name: "hour",
				queryMode: "local",
				store: Ext.create("Ext.data.ArrayStore", {
					fields: [ "value", "text" ],
					data: Date.mapHour
				}),
				displayField: "text",
				valueField: "value",
				allowBlank: false,
				editable: false,
				triggerAction: "all",
				value: String(new Date().getHours()),
				flex: 1
			},{
				xtype: "checkbox",
				name: "everynhour",
				fieldLabel: "",
				checked: false,
				boxLabel: _("Every N hour"),
				width: 140
			}]
		},{
			xtype: "compositefield",
			fieldLabel: _("Day of month"),
			combineErrors: false,
			items: [{
				xtype: "combo",
				name: "dayofmonth",
				queryMode: "local",
				store: Ext.create("Ext.data.ArrayStore", {
					fields: [ "value", "text" ],
					data: Date.mapDayOfMonth
				}),
				displayField: "text",
				valueField: "value",
				allowBlank: false,
				editable: false,
				triggerAction: "all",
				value: "*",
				flex: 1
			},{
				xtype: "checkbox",
				name: "everyndayofmonth",
				fieldLabel: "",
				checked: false,
				boxLabel: _("Every N day of month"),
				width: 140
			}]
		},{
			xtype: "combo",
			name: "month",
			fieldLabel: _("Month"),
			queryMode: "local",
			store: Ext.create("Ext.data.ArrayStore", {
				fields: [ "value", "text" ],
				data: Date.mapMonth
			}),
			displayField: "text",
			valueField: "value",
			allowBlank: false,
			editable: false,
			triggerAction: "all",
			value: "*"
		},{
			xtype: "combo",
			name: "dayofweek",
			fieldLabel: _("Day of week"),
			queryMode: "local",
			store: Ext.create("Ext.data.ArrayStore", {
				fields: [ "value", "text" ],
				data: Date.mapDayOfWeek
			}),
			displayField: "text",
			valueField: "value",
			allowBlank: false,
			editable: false,
			triggerAction: "all",
			value: "*"
		},{
			xtype: "usercombo",
			name: "username",
			fieldLabel: _("User"),
			value: "root"
		},{
			xtype: "textfield",
			name: "command",
			fieldLabel: _("Command"),
			allowBlank: false
		},{
			xtype: "checkbox",
			name: "sendemail",
			fieldLabel: _("Send email"),
			checked: false,
			boxLabel: _("Send command output via email"),
			plugins: [{
				ptype: "fieldinfo",
				text: _("An email message with the command output (if any produced) is send to the user who performs the job.")
			}]
		},{
			xtype: "textarea",
			name: "comment",
			fieldLabel: _("Comment"),
			allowBlank: true,
			vtype: "comment"
		},{
			xtype: "hidden",
			name: "type",
			value: "userdefined"
		}];
	},

	isValid: function() {
		var me = this;
		if(!me.callParent(arguments))
			return false;
		var valid = true;
		// It is not allowed to select '*' if the everyxxx checkbox
		// is checked.
		[ "minute", "hour", "dayofmonth" ].each(function(name) {
			var field = me.findField(name);
			field.clearInvalid(); // combineErrors is false
			if((field.getValue() === "*") && (me.findField(
			  "everyn" + name).checked)) {
				field.markInvalid(_("Ranges of numbers are not allowed"));
				valid = false;
			}
		});
		return valid;
	}
});

/**
 * @class OMV.module.admin.system.cron.Jobs
 * @derived OMV.workspace.grid.Panel
 */
Ext.define("OMV.module.admin.system.cron.Jobs", {
	extend: "OMV.workspace.grid.Panel",
	requires: [
		"OMV.Rpc",
		"OMV.data.Store",
		"OMV.data.Model",
		"OMV.data.proxy.Rpc",
		"OMV.util.Format",
		"OMV.window.Execute"
	],
	uses: [
		"OMV.module.admin.system.cron.Job"
	],
	
	hidePagingToolbar: false,
	stateful: true,
	stateId: "a882a76d-6804-4632-b31b-8b48c0ea6dde",
	columns: [{
		xtype: "booleaniconcolumn",
		text: _("Enabled"),
		sortable: true,
		dataIndex: "enable",
		stateId: "enable",
		align: "center",
		width: 80,
		resizable: false,
		trueIcon: "switch_on.png",
		falseIcon: "switch_off.png"
	},{
		text: _("Minute"),
		sortable: true,
		dataIndex: "minute",
		stateId: "minute",
		renderer: function(value, metaData, record) {
			var everynminute = record.get("everynminute");
			if(everynminute == true) {
				value = "*/" + value;
			}
			return value;
		}
	},{
		text: _("Hour"),
		sortable: true,
		dataIndex: "hour",
		stateId: "hour",
		renderer: function(value, metaData, record) {
			var everynhour = record.get("everynhour");
			var func = OMV.util.Format.arrayRenderer(Date.mapHour);
			value = func(value);
			if(everynhour == true) {
				value = "*/" + value;
			}
			return value;
		}
	},{
		text: _("Day of month"),
		sortable: true,
		dataIndex: "dayofmonth",
		stateId: "dayofmonth",
		renderer: function(value, metaData, record) {
			var everyndayofmonth = record.get("everyndayofmonth");
			var func = OMV.util.Format.arrayRenderer(
			  Date.mapDayOfMonth);
			value = func(value);
			if(everyndayofmonth == true) {
				value = "*/" + value;
			}
			return value;
		}
	},{
		text: _("Month"),
		sortable: true,
		dataIndex: "month",
		stateId: "month",
		renderer: OMV.util.Format.arrayRenderer(Date.mapMonth)
	},{
		text: _("Day of week"),
		sortable: true,
		dataIndex: "dayofweek",
		stateId: "dayofweek",
		renderer: OMV.util.Format.arrayRenderer(Date.mapDayOfWeek)
	},{
		text: _("User"),
		sortable: true,
		dataIndex: "username",
		stateId: "username"
	},{
		text: _("Command"),
		sortable: true,
		dataIndex: "command",
		stateId: "command"
	},{
		text: _("Comment"),
		sortable: true,
		dataIndex: "comment",
		stateId: "comment"
	}],

	initComponent: function() {
		var me = this;
		Ext.apply(me, {
			store: Ext.create("OMV.data.Store", {
				autoLoad: true,
				model: OMV.data.Model.createImplicit({
					idProperty: "uuid",
					fields: [
						{ name: "uuid", type: "string" },
						{ name: "enable", type: "boolean" },
						{ name: "type", type: "string" },
						{ name: "minute", type: "string" },
						{ name: "everynminute", type: "boolean" },
						{ name: "hour", type: "string" },
						{ name: "everynhour", type: "boolean" },
						{ name: "dayofmonth", type: "string" },
						{ name: "everyndayofmonth", type: "boolean" },
						{ name: "month", type: "string" },
						{ name: "dayofweek", type: "string" },
						{ name: "username", type: "string" },
						{ name: "command", type: "string" },
						{ name: "sendemail", type: "boolean" },
						{ name: "comment", type: "string" }
					]
				}),
				proxy: {
					type: "rpc",
					rpcData: {
						service: "Cron",
						method: "getList"
					},
					extraParams: {
						type: [ "userdefined" ]
					}
				}
			})
		});
		me.callParent(arguments);
	},

	getTopToolbarItems: function() {
		var me = this;
		var items = me.callParent(arguments);
		// Add 'Run' button to top toolbar.
		Ext.Array.insert(items, 2, [{
			id: me.getId() + "-run",
			xtype: "button",
			text: _("Run"),
			icon: "images/play.png",
			iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
			handler: Ext.Function.bind(me.onRunButton, me, [ me ]),
			scope: me,
			disabled: true
		}]);
		return items;
	},

	onSelectionChange: function(model, records) {
		var me = this;
		me.callParent(arguments);
		// Process additional buttons.
		var tbarRunCtrl = me.queryById(me.getId() + "-run");
		if(records.length <= 0) {
			tbarRunCtrl.disable();
		} else if(records.length == 1) {
			tbarRunCtrl.enable();
		} else {
			tbarRunCtrl.disable();
		}
	},

	onAddButton: function() {
		var me = this;
		Ext.create("OMV.module.admin.system.cron.Job", {
			title: _("Add scheduled job"),
			uuid: OMV.UUID_UNDEFINED,
			listeners: {
				scope: me,
				submit: function() {
					this.doReload();
				}
			}
		}).show();
	},

	onEditButton: function() {
		var me = this;
		var record = me.getSelected();
		Ext.create("OMV.module.admin.system.cron.Job", {
			title: _("Edit scheduled job"),
			uuid: record.get("uuid"),
			listeners: {
				scope: me,
				submit: function() {
					this.doReload();
				}
			}
		}).show();
	},

	doDeletion: function(record) {
		var me = this;
		OMV.Rpc.request({
			scope: me,
			callback: me.onDeletion,
			rpcData: {
				service: "Cron",
				method: "delete",
				params: {
					uuid: record.get("uuid")
				}
			}
		});
	},

	onRunButton: function() {
		var me = this;
		var record = me.getSelected();
		Ext.create("OMV.window.Execute", {
			title: _("Execute cron job"),
			rpcService: "Cron",
			rpcMethod: "execute",
			rpcParams: {
				uuid: record.get("uuid")
			},
			listeners: {
				scope: me,
				exception: function(wnd, error) {
					OMV.MessageBox.error(null, error);
				}
			}
		}).show();
	}
});

OMV.WorkspaceManager.registerNode({
	id: "cron",
	path: "/system",
	text: _("Cron Jobs"),
	icon16: "images/alarm.png",
	iconSvg: "images/alarm.svg",
	position: 70
});

OMV.WorkspaceManager.registerPanel({
	id: "scheduledjobs",
	path: "/system/cron",
	text: _("Scheduled jobs"),
	position: 10,
	className: "OMV.module.admin.system.cron.Jobs"
});
