/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2014 Volker Theile
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
// require("js/omv/workspace/panel/Panel.js")
// require("js/omv/workspace/node/Model.js")

/**
 * Display all registered workspace nodes in an accordion panel.
 * @class OMV.workspace.node.panel.Panel
 * @derived OMV.workspace.panel.Panel
 */
Ext.define("OMV.workspace.node.panel.Panel", {
	extend: "OMV.workspace.panel.Panel",
	alias: "widget.workspacenodepanel",
	requires: [
		"OMV.WorkspaceManager",
		"OMV.workspace.node.Model"
	],
	uses: [
		"Ext.data.Store",
		"Ext.view.View",
		"Ext.XTemplate"
	],

	constructor: function(config) {
		var me = this;
		config = Ext.apply({
			root: OMV.WorkspaceManager.getRootNode()
		}, config || {});
		me.callParent([ config ]);
	},

	initComponent: function() {
		var me = this;
		Ext.apply(me, {
			items: Ext.create("Ext.view.View", {
				multiSelect: false,
				trackOver: true,
				overItemCls: Ext.baseCSSPrefix + "item-over",
				itemSelector: "div.thumb-wrap",
				store: Ext.create("Ext.data.Store", {
					model: "OMV.workspace.node.Model",
					data: me.root.getRange()
				}),
				tpl: Ext.create("Ext.XTemplate",
					'<div class="',Ext.baseCSSPrefix,'workspace-node-view">',
						'<tpl for=".">',
							'<div class="thumb-wrap" id="{id:stripTags}">',
								'<div class="thumb"><img src="{[this.renderIcon(values)]}" title="{text:htmlEncode}"></div>',
								'<span>{text:htmlEncode}</span>',
							'</div>',
						'</tpl>',
					'</div>',
					{
						renderIcon: function(values) {
							var node = Ext.create("OMV.workspace.node.Node",
							  values);
							return node.getIcon32();
						}
					}
				)
			})
		});
		me.callParent(arguments);
	}
});
