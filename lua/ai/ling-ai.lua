neoluoyi_skill = {}
neoluoyi_skill.name = "neoluoyi"
table.insert(sgs.ai_skills, neoluoyi_skill)
neoluoyi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("LuoyiCard") then return nil end
	if self:needBear() then return nil end
	local luoyicard
	if self:needToThrowArmor() then
		luoyicard = self.player:getArmor()
		return sgs.Card_Parse("@LuoyiCard=" .. luoyicard:getEffectiveId())
	end
	
	if not self:slashIsAvailable(self.player) then return nil end
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	local slashtarget = 0
	local dueltarget = 0
	local equipnum = 0
	local offhorse = self.player:getOffensiveHorse()
	local noHorseTargets = 0
	self:sort(self.enemies,"hp")
	for _, card in sgs.qlist(self.player:getCards("he")) do
		if card:isKindOf("EquipCard") and not (card:isKindOf("Weapon") and self.player:hasEquip(card)) then
			equipnum = equipnum + 1
		end
	end
	for _,card in ipairs(cards) do
		if card:isKindOf("Slash") then
			for _,enemy in ipairs(self.enemies) do
				if self.player:canSlash(enemy, card) and self:slashIsEffective(card, enemy) and self:objectiveLevel(enemy) > 3 and sgs.isGoodTarget(enemy, self.enemies, self, true) then
					if getCardsNum("Jink", enemy) < 1 or (self:isEquip("Axe") and self.player:getCards("he"):length() > 4) then
						slashtarget = slashtarget + 1
						if offhorse and self.player:distanceTo(enemy, 1)<=self.player:getAttackRange() then
							noHorseTargets = noHorseTargets + 1
						end
					end
				end
			end
		end
		if card:isKindOf("Duel") then
			for _, enemy in ipairs(self.enemies) do
				if self:getCardsNum("Slash") >= getCardsNum("Slash", enemy) and sgs.isGoodTarget(enemy, self.enemies, self)
				and self:objectiveLevel(enemy) > 3 and not self:cantbeHurt(enemy, 2) and self:damageIsEffective(enemy) and enemy:getMark("@late") == 0 then
					dueltarget = dueltarget + 1 
				end
			end
		end
	end		
	if (slashtarget + dueltarget) > 0 and equipnum > 0 then
		self:speak("luoyi")
		if self:needToThrowArmor() then
			luoyicard = self.player:getArmor()
		end
		
		if not luoyicard then
			for _, card in sgs.qlist(self.player:getCards("he")) do
				if card:isKindOf("EquipCard") and not self.player:hasEquip(card) then 
					luoyicard = card
					break
				end
			end
		end
		if not luoyicard and offhorse then
			if noHorseTargets == 0 then
				for _, card in sgs.qlist(self.player:getCards("he")) do
					if card:isKindOf("EquipCard") and not card:isKindOf("OffensiveHorse") then 
						luoyicard = card
						break
					end
				end
				if not luoyicard and dueltarget == 0 then return nil end
			end
		end
		if not luoyicard then
			for _, card in sgs.qlist(self.player:getCards("he")) do
				if card:isKindOf("EquipCard") and not (card:isKindOf("Weapon") and self.player:hasEquip(card)) then
					luoyicard = card
					break
				end
			end
		end
		if not luoyicard then return nil end
		return sgs.Card_Parse("@LuoyiCard=" .. luoyicard:getEffectiveId())
	end
end

sgs.ai_skill_use_func.LuoyiCard=function(card,use,self)
	use.card = card
end

sgs.ai_use_priority.LuoyiCard = 9.2

local neofanjian_skill = {}
neofanjian_skill.name = "neofanjian"
table.insert(sgs.ai_skills, neofanjian_skill)
neofanjian_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() then return nil end
	if self.player:hasUsed("NeoFanjianCard") then return nil end
	return sgs.Card_Parse("@NeoFanjianCard=.")
end

sgs.ai_skill_use_func.NeoFanjianCard = function(card, use, self)
	self:sort(self.enemies, "defense")
	local target
	for _, enemy in ipairs(self.enemies) do
		if self:canAttack(enemy) and not self:hasSkills("qingnang|tianxiang", enemy) then
			target = enemy

			local wuguotai = self.room:findPlayerBySkillName("buyi")
			local care = (target:getHp() <= 1) and (self:isFriend(target, wuguotai))
			local ucard = nil
			local handcards = self.player:getCards("h")
			handcards = sgs.QList2Table(handcards)
			self:sortByKeepValue(handcards)
			for _,cd in ipairs(handcards) do
				local flag = not (cd:isKindOf("Peach") or cd:isKindOf("Analeptic"))
				local suit = cd:getSuit()
				if flag and care then
					flag = cd:isKindOf("BasicCard")
				end
				if flag and target:hasSkill("longhun") then
					flag = (suit ~= sgs.Card_Heart)
				end
				if flag and target:hasSkill("jiuchi") then
					flag = (suit ~= sgs.Card_Spade)
				end
				if flag and target:hasSkill("jijiu") then
					flag = (cd:isBlack())
				end
				if flag then
					ucard = cd
					break
				end
			end
			if ucard then
				local keep_value = self:getKeepValue(ucard)
				if ucard:getSuit() == sgs.Card_Diamond then keep_value = keep_value + 0.5 end
				if keep_value < 6 then
					use.card = sgs.Card_Parse("@NeoFanjianCard=" .. ucard:getEffectiveId())
					if use.to then use.to:append(target) end
					return
				end
			end
		end
	end
end

sgs.ai_card_intention.NeoFanjianCard = sgs.ai_card_intention.FanjianCard
sgs.dynamic_value.damage_card.NeoFanjianCard = true

function sgs.ai_skill_suit.neofanjian(self)
	local map = {0, 0, 1, 2, 2, 3, 3, 3}
	local suit = map[math.random(1, 8)]
	if self.player:hasSkill("hongyan") and suit == sgs.Card_Spade then return sgs.Card_Heart else return suit end
end

sgs.ai_skill_invoke.yishi = function(self, data)
	local damage = data:toDamage()
	local target = damage.to
	if self:isFriend(target) then
		if damage.damage == 1 and self:getDamagedEffects(target, self.player)
			and (target:getJudgingArea():isEmpty() or target:containsTrick("YanxiaoCard")) then
			return false
		end
		return true
	else
		if self:hasHeavySlashDamage(self.player, damage.card, target) then return false end
		if self:isWeak(target) then return false end
		if self:doNotDiscard(target, "e", true) then
			return false
		end
		if self:getDamagedEffects(target, self.player, true) or (target:getArmor() and not target:getArmor():isKindOf("SilverLion")) then return true end
		if self:getDangerousCard(target) then return true end
		if target:getDefensiveHorse() then return true end
		return false
	end 
end

sgs.ai_skill_invoke.zhulou = function(self, data)
	local weaponnum = 0
	for _, card in sgs.qlist(self.player:getCards("h")) do
		if card:isKindOf("Weapon") then
			weaponnum = weaponnum + 1
		end
	end

	if weaponnum > 0 then return true end
		
	if self.player:getHandcardNum() < 3 and self.player:getHp() > 2 then
		return true
	end

	if self.player:getHp() < 3 and self.player:getWeapon() then
		return true
	end

	return false
end

sgs.ai_skill_cardask["@zhulou-discard"] =  function(self, data, pattern)
	local ccards = sgs.QList2Table(self.player:getCards("he"))
	local lightning = self:getCard("Lightning")
	self:sortByCardNeed(ccards)
	if pattern == ".Weapon" then
		if self.player:getWeapon() then return "$" .. self.player:getWeapon():getEffectiveId() end
		for _, card in sgs.list(ccards) do
			if card:isKindOf("Weapon") then return "$" .. card:getEffectiveId() end
		end
	elseif pattern == "^BasicCard" then
		if self:needToThrowArmor() then return "$" .. self.player:getArmor():getEffectiveId() end
		if lightning then return lightning:getEffectiveId() end
		for _, card in sgs.list(ccards) do
			if card:getTypeId() ~= sgs.Card_TypeBasic then return "$" .. card:getEffectiveId() end
		end
		return "."
	end
	return "."
end


sgs.ai_cardneed.zhulou = sgs.ai_cardneed.weapon

sgs.zhulou_keep_value = sgs.qiangxi_keep_value

function sgs.ai_skill_invoke.neojushou(self, data)
	if not self.player:faceUp() then return true end
	for _, friend in ipairs(self.friends) do
		if self:hasSkills("fangzhu|jilve", friend) then return true end
	end
	return self:isWeak()
end

sgs.ai_skill_invoke.neoganglie = function(self, data)
	local damage = data:toDamage()
	if not damage.from then
		local zhangjiao = self.room:findPlayerBySkillName("guidao")
		return zhangjiao and self:isFriend(zhangjiao)
	end
	local who = damage.from
	if self:isFriend(who) and (self:getDamagedEffects(who, self.player) or self:needToLoseHp(who, self.player, nil, true)) then
		who:setFlags("ganglie_target")
		return true
	end
	if self:getDamagedEffects(who, self.player) and self:isEnemy(who) and who:getHandcardNum() < 2 then 
		return false
	end
	
	return not self:isFriend(who)
end

sgs.ai_choicemade_filter.skillInvoke.neoganglie = function(player, promptlist, self)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.from then
		local target = damage.from
		local intention = 10
		if promptlist[3] == "yes" then
			if self:getDamagedEffects(target, player) or self:needToLoseHp(target, player, nil, true) then
				intention = 0
			end
			sgs.updateIntention(player, target, intention)
		elseif self:canAttack(target) then
			sgs.updateIntention(player, target, -10)
		end
	end
end

sgs.ai_need_damaged.neoganglie = function (self, attacker, player)
	if not player:hasSkill("neoganglie") then return false end
	if self:isEnemy(attacker, player) and attacker:getHp() <= 2 and not hasBuquEffect(attacker) and sgs.isGoodTarget(attacker, self.enemies, self)
		and not self:getDamagedEffects(attacker, player) and not self:needToLoseHp(attacker, player) then
			return true
	end
	return false
end

sgs.ai_skill_choice.neoganglie = function(self, choices)
	local target
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if player:hasFlag("ganglie_target") then
			target = player
			target:setFlags("-ganglie_target")
		end
	end
	if (self:getDamagedEffects(target, self.player) or self:needToLoseHp(target, self.player)) and self:isFriend(target) then return "damage" end

	if (self:getDamagedEffects(target, self.player) or self:needToLoseHp(target, self.player)) and target:getHandcardNum() > 1 then
		return "throw"
	end
	return "damage"
end

sgs.ai_skill_discard.neoganglie = function(self, discard_num, min_num, optional, include_equip)
	local to_discard = {}
	local cards = sgs.QList2Table(self.player:getHandcards())
	local index = 0
	self:sortByKeepValue(cards)
	cards = sgs.reverse(cards)

	for i = #cards, 1, -1 do
		local card = cards[i]
		if not self.player:isJilei(card) then
			table.insert(to_discard, card:getEffectiveId())
			table.remove(cards, i)
			index = index + 1
			if index == 2 then break end
		end
	end	
	return to_discard
end

sgs.ai_suit_priority.yishi= "club|spade|diamond|heart"


function SmartAI:useCardAwaitExhausted(card, use)
	use.card = card
	for _, player in ipairs(self.friends_noself) do
		if use.to and not player:hasSkill("manjuan") and not self.room:isProhibited(self.player, player, card) then
			use.to:append(player)
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if use.to and enemy:hasSkill("manjuan") and not self.room:isProhibited(self.player, enemy, card) then
			use.to:append(enemy)
		end
	end
end
sgs.ai_use_value.AwaitExhausted = sgs.ai_use_value.DuoshiCard
sgs.ai_use_priority.AwaitExhausted = sgs.ai_use_priority.DuoshiCard
sgs.ai_card_intention.AwaitExhausted = sgs.ai_card_intention.DuoshiCard

function SmartAI:useCardBefriendAttacking(card, use)
	local targets_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)
	local target = sgs.SPlayerList()
	self:sort(self.friends_noself, "defense")
	self:sort(self.enemies, "threat")
	local distance = 0
	local siblings = self.room:getOtherPlayers(self.player)
    for _, p in sgs.list(siblings) do
        local dist = self.player:distanceTo(p)
        if (dist > distance) then distance = dist end
    end
	for _, friend in ipairs(self.friends_noself) do
		if not self.room:isProhibited(self.player, friend, card) and self:hasTrickEffective(card, friend) and
			self.player:distanceTo(friend) == distance then target:append(friend) end
		if target:length() >= targets_num then break end	
	end
	if target:length() == 0 then
	for _, en in ipairs(self.enemies) do
		if not self.room:isProhibited(self.player, en, card) and self:hasTrickEffective(card, en) and
			self.player:distanceTo(en) == distance and en:hasSkill("manjuan") then target:append(en) break end
		if not self.room:isProhibited(self.player, en, card) and self:hasTrickEffective(card, en) and
			self.player:distanceTo(en) == distance then target:append(en) break end
	end
	end
	if target:length() > 0 then
		use.card = card
		if use.to then use.to = target end
		return
	end
end
sgs.ai_card_intention.BefriendAttacking = -50
sgs.ai_use_value.BefriendAttacking = 7.8
sgs.ai_use_priority.BefriendAttacking = 4.35

function SmartAI:useCardKnownBoth(card, use)
	use.card = card
	return
end
sgs.ai_use_priority.KnownBoth = 8
sgs.ai_use_value.KnownBoth = sgs.ai_use_value.AmazingGrace - 1


sgs.weapon_range.SixSwords = 2
sgs.weapon_range.Triblade = 3
sgs.weapon_range.DragonPhoenix = 2

sgs.ai_skill_use["@@SixSwords"] = function(self, prompt)
	local targets = {}
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends_noself) do table.insert(targets, friend:objectName()) end
	if #targets == 0 then return "." else return "@SixSwordsSkillCard=.->" .. table.concat(targets, "+") end
end

sgs.ai_skill_use["@@Triblade"]=function(self,prompt)

	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	self:sortByCardNeed(cards)
	local cdid
	local tar
	local lightnings = self:getCards("Lightning", "h")
	for _,lightning in ipairs(lightnings) do
		if lightning and not self:willUseLightning(lightning) then cdid = lightning:getEffectiveId() break end
	end
	if not cdid and self.player:getHandcardNum() >= self.player:getHp() - 1 then 
		for _,card in ipairs(cards) do
			if not isCard("Peach", card, self.player) and not isCard("Analeptic", card, self.player) then cdid = card:getEffectiveId() break end
		end
	end
	if not cdid or self.player:isKongcheng() then return "." end
	self:sort(self.enemies, "hp")
	self:sort(self.friends, "threat")
	for _,e in ipairs(self.enemies) do 
		if e:hasFlag("TribladeFilter") and not self:getDamagedEffects(e, self.player) and self:canAttack(e) then tar = e break end
	end
	if not tar then
		for _,f in ipairs(self.friends) do 
			if f:hasFlag("TribladeFilter") and self:getDamagedEffects(f, self.player) and not self:isWeak(f) then tar = f break end
		end
	end
	if tar then return "@TribladeSkillCard="..cdid.."->"..tar:objectName() end
	return "."
end
sgs.ai_card_intention.TribladeSkillCard = 30


sgs.ai_skill_invoke.DragonPhoenix = function(self, data)
	local use = data:toCardUse()
	if use then
		for _,to in sgs.qlist(use.to) do 
			if self:isFriend(to) and self:doNotDiscard(to) then return true end
			if self:isEnemy(to) and not self:doNotDiscard(to) then return true end
		end
		return false
	end
	return false
end

sgs.ai_skill_invoke.neo2013renwang = sgs.ai_skill_invoke.danlao
	
local neo2013xinzhan_skill={}
neo2013xinzhan_skill.name="neo2013xinzhan"
table.insert(sgs.ai_skills,neo2013xinzhan_skill)
neo2013xinzhan_skill.getTurnUseCard=function(self)
	if not self.player:hasUsed("Neo2013XinzhanCard") and self.player:getHandcardNum() > self.player:getLostHp() then
		return sgs.Card_Parse("@Neo2013XinzhanCard=.")
	end
end

sgs.ai_skill_use_func.Neo2013XinzhanCard=function(card,use,self)
	use.card = card
end

sgs.ai_use_value.Neo2013XinzhanCard = 4.5
sgs.ai_use_priority.Neo2013XinzhanCard = 9.5

sgs.ai_slash_prohibit.neo2013huilei = sgs.ai_slash_prohibit.huilei

sgs.ai_skill_invoke.neo2013yishi = sgs.ai_skill_invoke.yishi 

function sgs.ai_cardsview.neo2013haoyin(self, class_name, player)
	if class_name == "Analeptic" and player:hasSkill("neo2013haoyin") and sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_PLAY then
		if player:getHp() < 2 or self:isWeak(player) then return nil end
		return ("analeptic:neo2013haoyin[no_suit:0]=.")
	end
end

sgs.ai_skill_invoke.neo2013zhulou = sgs.ai_skill_invoke.zhulou
sgs.ai_cardneed.neo2013zhulou = sgs.ai_cardneed.weapon
sgs.neo2013zhulou_keep_value = sgs.qiangxi_keep_value

local neo2013fanjian_skill = {}
neo2013fanjian_skill.name = "neo2013fanjian"
table.insert(sgs.ai_skills, neo2013fanjian_skill)
neo2013fanjian_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() or self.player:hasUsed("Neo2013FanjianCard") then return nil end
	return sgs.Card_Parse("@Neo2013FanjianCard=.")
end

sgs.ai_skill_use_func.Neo2013FanjianCard = function(card, use, self)
	self:sort(self.enemies, "defense")
	local target
	for _, enemy in ipairs(self.enemies) do
		if self:canAttack(enemy) and not self:hasSkills("qingnang|tianxiang", enemy) then
			target = enemy

			local wuguotai = self.room:findPlayerBySkillName("buyi")
			local care = (target:getHp() <= 1) and (self:isFriend(target, wuguotai))
			local ucard = nil
			local handcards = self.player:getCards("h")
			handcards = sgs.QList2Table(handcards)
			self:sortByKeepValue(handcards)
			for _,cd in ipairs(handcards) do
				local flag = not (cd:isKindOf("Peach") or cd:isKindOf("Analeptic"))
				local suit = cd:getSuit()
				if flag and care then
					flag = cd:isKindOf("BasicCard")
				end
				if flag and target:hasSkill("longhun") then
					flag = (suit ~= sgs.Card_Heart)
				end
				if flag and target:hasSkill("jiuchi") then
					flag = (suit ~= sgs.Card_Spade)
				end
				if flag and target:hasSkill("jijiu") then
					flag = (cd:isBlack())
				end
				if flag then
					ucard = cd
					break
				end
			end
			if ucard then
				local keep_value = self:getKeepValue(ucard)
				if ucard:getSuit() == sgs.Card_Diamond then keep_value = keep_value + 0.5 end
				if keep_value < 6 then
					use.card = sgs.Card_Parse("@Neo2013FanjianCard=" .. ucard:getEffectiveId())
					if use.to then use.to:append(target) end
					return
				end
			end
		end
	end
end

sgs.ai_card_intention.Neo2013FanjianCard = sgs.ai_card_intention.FanjianCard

function sgs.ai_skill_suit.neo2013fanjian(self)
	local map = {0, 0, 1, 2, 2, 3, 3, 3}
	local suit = map[math.random(1, 8)]
	if self.player:hasSkill("hongyan") and suit == sgs.Card_Spade then return sgs.Card_Heart else return suit end
end

sgs.ai_skill_playerchosen.neo2013fankui = function(self, targets)
	local to
	local player = self:findPlayerToDiscard("he", false, false, targets)
	targets = sgs.QList2Table(targets)
	self:sort(self.friends_noself, "threat")
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if not self:doNotDiscard(enemy) or self:getOverflow(fr) >= 0 then to = enemy break end
	end
	if not to then
		for _, fr in ipairs(self.friends_noself) do
			if self:doNotDiscard(fr) or self:getOverflow(fr) > 2 then to = fr break end
		end
	end
	return player or to or nil
end
sgs.ai_playerchosen_intention.neo2013fankui = 30
