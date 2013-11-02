#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"

class Jianxiong: public MasochismSkill {
public:
    Jianxiong(): MasochismSkill("jianxiong") {
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const{
        Room *room = caocao->getRoom();
        const Card *card = damage.card;
        if (card && room->getCardPlace(card->getEffectiveId()) == Player::PlaceTable) {
            QVariant data = QVariant::fromValue(card);
            if (room->askForSkillInvoke(caocao, "jianxiong", data)) {
                room->broadcastSkillInvoke(objectName());
                caocao->obtainCard(card);
            }
        }
    }
};

class Hujia: public TriggerSkill {
public:
    Hujia(): TriggerSkill("hujia$") {
        events << CardAsked;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("hujia");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caocao, QVariant &data) const{
        QString pattern = data.toStringList().first();
        QString prompt = data.toStringList().at(1);
        if (pattern != "jink" || prompt.startsWith("@hujia-jink"))
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("wei", caocao);
        if (lieges.isEmpty())
            return false;

        if (!room->askForSkillInvoke(caocao, objectName(), data))
            return false;

        if (caocao->hasSkill("weidi"))
            room->broadcastSkillInvoke("weidi");
        else
            room->broadcastSkillInvoke(objectName());

        QVariant tohelp = QVariant::fromValue((PlayerStar)caocao);
        foreach (ServerPlayer *liege, lieges) {
            const Card *jink = room->askForCard(liege, "jink", "@hujia-jink:" + caocao->objectName(),
                                                tohelp, Card::MethodResponse, caocao, false, QString(), true);
            if (jink) {
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

class TuxiViewAsSkill: public ZeroCardViewAsSkill {
public:
    TuxiViewAsSkill(): ZeroCardViewAsSkill("tuxi") {
        response_pattern = "@@tuxi";
    }

    virtual const Card *viewAs() const{
        return new TuxiCard;
    }
};

class Tuxi: public PhaseChangeSkill {
public:
    Tuxi(): PhaseChangeSkill("tuxi") {
        view_as_skill = new TuxiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhangliao) const{
        if (zhangliao->getPhase() == Player::Draw) {
            Room *room = zhangliao->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(zhangliao);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke && room->askForUseCard(zhangliao, "@@tuxi", "@tuxi-card"))
                return true;
        }

        return false;
    }
};

class Tiandu: public TriggerSkill {
public:
    Tiandu(): TriggerSkill("tiandu") {
        frequency = Frequent;
        events << FinishJudge;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        CardStar card = judge->card;

        QVariant data_card = QVariant::fromValue(card);
        if (guojia->askForSkillInvoke(objectName(), data_card)) {
            room->broadcastSkillInvoke(objectName());
            guojia->obtainCard(judge->card);
            return false;
        }

        return false;
    }
};

Yiji::Yiji(): MasochismSkill("yiji") {
    frequency = Frequent;
    n = 2;
}

void Yiji::onDamaged(ServerPlayer *guojia, const DamageStruct &damage) const{
    Room *room = guojia->getRoom();
    int x = damage.damage;
    for (int i = 0; i < x; i++) {
        if (!guojia->isAlive() || !room->askForSkillInvoke(guojia, objectName()))
            return;
        room->broadcastSkillInvoke("yiji");

        QList<ServerPlayer *> _guojia;
        _guojia.append(guojia);
        QList<int> yiji_cards = room->getNCards(n, false);

        CardsMoveStruct move(yiji_cards, NULL, guojia, Player::PlaceTable, Player::PlaceHand,
                             CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
        QList<CardsMoveStruct> moves;
        moves.append(move);
        room->notifyMoveCards(true, moves, false, _guojia);
        room->notifyMoveCards(false, moves, false, _guojia);

        QList<int> origin_yiji = yiji_cards;
        while (room->askForYiji(guojia, yiji_cards, objectName(), true, false, true, -1, room->getAlivePlayers())) {
            CardsMoveStruct move(QList<int>(), guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            foreach (int id, origin_yiji) {
                if (room->getCardPlace(id) != Player::DrawPile) {
                    move.card_ids << id;
                    yiji_cards.removeOne(id);
                }
            }
            origin_yiji = yiji_cards;
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);
            if (!guojia->isAlive())
                return;
        }

        if (!yiji_cards.isEmpty()) {
            CardsMoveStruct move(yiji_cards, guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);

            foreach (int id, yiji_cards)
                guojia->obtainCard(Sanguosha->getCard(id), false);
        }
    }
}

class Ganglie: public MasochismSkill {
public:
    Ganglie(): MasochismSkill("ganglie") {
    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();
        QVariant data = QVariant::fromValue(damage);

        if (room->askForSkillInvoke(xiahou, "ganglie", data)) {
            room->broadcastSkillInvoke(objectName());

            JudgeStruct judge;
            judge.pattern = ".|heart";
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if (!from || from->isDead()) return;
            if (judge.isGood()) {
                if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
                    room->damage(DamageStruct(objectName(), xiahou, from));
            }
        }
    }
};

class Fankui: public MasochismSkill {
public:
    Fankui(): MasochismSkill("fankui") {
    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = simayi->getRoom();
        QVariant data = QVariant::fromValue(from);
        if (from && !from->isNude() && room->askForSkillInvoke(simayi, "fankui", data)) {
            room->broadcastSkillInvoke(objectName());
            int card_id = room->askForCardChosen(simayi, from, "he", "fankui");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
            room->obtainCard(simayi, Sanguosha->getCard(card_id),
                             reason, room->getCardPlace(card_id) != Player::PlaceHand);
        }
    }
};

class Guicai: public TriggerSkill {
public:
    Guicai(): TriggerSkill("guicai") {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isKongcheng())
            return false;

        JudgeStar judge = data.value<JudgeStar>();

        QStringList prompt_list;
        prompt_list << "@guicai-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        bool forced = false;
        if (player->getMark("JilveEvent") == int(AskForRetrial))
            forced = true;
        const Card *card = room->askForCard(player, forced ? ".!" : "." , prompt, data, Card::MethodResponse, judge->who, true);
        if (forced && card == NULL)
            card = player->getRandomHandCard();
        if (card) {
            if (player->hasInnateSkill("guicai") || !player->hasSkill("jilve"))
                room->broadcastSkillInvoke(objectName());
            else
                room->broadcastSkillInvoke("jilve", 1);
            room->retrial(card, player, judge, objectName());
        }

        return false;
    }
};

class LuoyiBuff: public TriggerSkill {
public:
    LuoyiBuff(): TriggerSkill("#luoyi") {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("luoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class Luoyi: public DrawCardsSkill {
public:
    Luoyi(): DrawCardsSkill("luoyi") {
    }

    virtual int getDrawNum(ServerPlayer *xuchu, int n) const{
        Room *room = xuchu->getRoom();
        if (room->askForSkillInvoke(xuchu, objectName())) {
            room->broadcastSkillInvoke(objectName());
            xuchu->setFlags(objectName());
            return n - 1;
        } else
            return n;
    }
};

class Luoshen: public TriggerSkill {
public:
    Luoshen(): TriggerSkill("luoshen") {
        events << EventPhaseStart << FinishJudge;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhenji, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && zhenji->getPhase() == Player::Start) {
            while (zhenji->askForSkillInvoke("luoshen")) {
                room->broadcastSkillInvoke(objectName());

                JudgeStruct judge;
                judge.pattern = ".|black";
                judge.good = true;
                judge.reason = objectName();
                judge.play_animation = false;
                judge.who = zhenji;
                judge.time_consuming = true;

                room->judge(judge);
                if (judge.isBad())
                    break;
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == objectName()) {
                if (judge->card->isBlack()) {
                    if (Config.EnableHegemony) {
                        CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, zhenji->objectName(), QString(), judge->reason);
                        room->moveCardTo(judge->card, zhenji, NULL, Player::PlaceTable, reason, true);
                        QVariantList luoshen_list = zhenji->tag[objectName()].toList();
                        luoshen_list << judge->card->getEffectiveId();
                        zhenji->tag[objectName()] = luoshen_list;
                    } else {
                        zhenji->obtainCard(judge->card);
                    }
                }
                else if (Config.EnableHegemony) {
                    DummyCard *dummy = new DummyCard(VariantList2IntList(zhenji->tag[objectName()].toList()));
                    zhenji->obtainCard(dummy);
                    zhenji->tag.remove(objectName());
                    delete dummy;
                }
            }
        }

        return false;
    }
};

class Qingguo: public OneCardViewAsSkill {
public:
    Qingguo(): OneCardViewAsSkill("qingguo") {
        filter_pattern = ".|black|.|hand";
        response_pattern = "jink";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        return jink;
    }
};

class Rende: public ZeroCardViewAsSkill{
public:
    Rende(): ZeroCardViewAsSkill("rende"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("RendeCard");
    }

    virtual const Card *viewAs() const{
        return new RendeCard;
    }
};

/*
class RendeViewAsSkill: public ViewAsSkill {
public:
    RendeViewAsSkill(): ViewAsSkill("rende") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("rende") >= 2)
           return false;
        else {
            if (to_select->isEquipped()) return false;
            if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@rende") {
                QList<int> rende_list = StringList2IntList(Self->property("rende").toString().split("+"));
                return rende_list.contains(to_select->getEffectiveId());
            } else
                return true;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("rende") >= 2)
           return false;
        return !player->hasUsed("RendeCard") && !player->isKongcheng();
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@rende";
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        RendeCard *rende_card = new RendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class Rende: public TriggerSkill {
public:
    Rende(): TriggerSkill("rende") {
        events << EventPhaseChanging;
        view_as_skill = new RendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("rende") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "rende", 0);
        room->setPlayerProperty(player, "rende", QString());
        return false;
    }
};
*/

JijiangViewAsSkill::JijiangViewAsSkill(): ZeroCardViewAsSkill("jijiang$") {
}

bool JijiangViewAsSkill::isEnabledAtPlay(const Player *player) const{
    return hasShuGenerals(player) && !player->hasFlag("Global_JijiangFailed") && Slash::IsAvailable(player);
}

bool JijiangViewAsSkill::isEnabledAtResponse(const Player *player, const QString &pattern) const{
    return hasShuGenerals(player)
           && (pattern == "slash" || pattern == "@jijiang")
           && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
           && !player->hasFlag("Global_JijiangFailed");
}

const Card *JijiangViewAsSkill::viewAs() const{
    return new JijiangCard;
}

bool JijiangViewAsSkill::hasShuGenerals(const Player *player) {
    foreach (const Player *p, player->getAliveSiblings())
        if (p->getKingdom() == "shu")
            return true;
    return false;
}

class Jijiang: public TriggerSkill {
public:
    Jijiang(): TriggerSkill("jijiang$") {
        events << CardAsked;
        view_as_skill = new JijiangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("jijiang");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *liubei, QVariant &data) const{
        QString pattern = data.toStringList().first();
        QString prompt = data.toStringList().at(1);
        if (pattern != "slash" || prompt.startsWith("@jijiang-slash"))
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("shu", liubei);
        if (lieges.isEmpty())
            return false;

        if (!room->askForSkillInvoke(liubei, objectName(), data))
            return false;
        if (liubei->hasSkill("weidi"))
            room->broadcastSkillInvoke("weidi");
        else
            room->broadcastSkillInvoke(objectName(), getEffectIndex(liubei, NULL));

        foreach (ServerPlayer *liege, lieges) {
            const Card *slash = room->askForCard(liege, "slash", "@jijiang-slash:" + liubei->objectName(),
                                                 QVariant(), Card::MethodResponse, liubei, false, QString(), true);
            if (slash) {
                room->provide(slash);
                return true;
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int r = 1 + qrand() % 2;
        if (!player->hasInnateSkill("jijiang") && player->hasSkill("ruoyu"))
            r += 2;
        return r;
    }
};

class Wusheng: public OneCardViewAsSkill {
public:
    Wusheng(): OneCardViewAsSkill("wusheng") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *card) const{
        if (!card->isRed())
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(card->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        return slash;
    }
};

class Paoxiao: public TargetModSkill {
public:
    Paoxiao(): TargetModSkill("paoxiao") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Longdan: public OneCardViewAsSkill {
public:
    Longdan(): OneCardViewAsSkill("longdan") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
                return card->isKindOf("Jink");
            }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if (pattern == "slash")
                    return card->isKindOf("Jink");
                else if (pattern == "jink")
                    return card->isKindOf("Slash");
            }
        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else
            return NULL;
    }
};

class Tieji: public TriggerSkill {
public:
    Tieji(): TriggerSkill("tieji") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (player != use.from || !use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());

                p->setFlags("TiejiTarget"); // For AI

                JudgeStruct judge;
                judge.pattern = ".|red";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                try {
                    room->judge(judge);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        p->setFlags("-TiejiTarget");
                    throw triggerEvent;
                }

                if (judge.isGood()) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }

                p->setFlags("-TiejiTarget");
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class Guanxing: public PhaseChangeSkill {
public:
    Guanxing(): PhaseChangeSkill("guanxing") {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge) const{
        if (zhuge->getPhase() == Player::Start && zhuge->askForSkillInvoke(objectName())) {
            Room *room = zhuge->getRoom();
            int index = qrand() % 2 + 1;
            if (objectName() == "guanxing" && !zhuge->hasInnateSkill(objectName()) && zhuge->hasSkill("zhiji"))
                index += 2;
            room->broadcastSkillInvoke(objectName(), index);
            QList<int> guanxing = room->getNCards(getGuanxingNum(room));

            LogMessage log;
            log.type = "$ViewDrawPile";
            log.from = zhuge;
            log.card_str = IntList2StringList(guanxing).join("+");
            room->doNotify(zhuge, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

            room->askForGuanxing(zhuge, guanxing, false);
        }

        return false;
    }

    virtual int getGuanxingNum(Room *room) const{
        return qMin(5, room->alivePlayerCount());
    }
};

class Kongcheng: public ProhibitSkill {
public:
    Kongcheng(): ProhibitSkill("kongcheng") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("Slash") || card->isKindOf("Duel")) && to->isKongcheng();
    }
};

class KongchengEffect: public TriggerSkill {
public:
    KongchengEffect() :TriggerSkill("#kongcheng-effect") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isKongcheng()) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceHand))
                room->broadcastSkillInvoke("kongcheng");
        }

        return false;
    }
};

class Jizhi: public TriggerSkill {
public:
    Jizhi(): TriggerSkill("jizhi") {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->getTypeId() == Card::TypeTrick
            && (yueying->getMark("JilveEvent") > 0 || room->askForSkillInvoke(yueying, objectName()))) {
            if (yueying->getMark("JilveEvent") > 0)
                room->broadcastSkillInvoke("jilve", 5);
            else
                room->broadcastSkillInvoke(objectName());

            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, yueying, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, yueying->objectName(), "jizhi", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            if (!card->isKindOf("BasicCard")) {
                yueying->obtainCard(card);
            } else {
                const Card *card_ex = NULL;
                if (!yueying->isKongcheng())
                    card_ex = room->askForCard(yueying, ".", "@jizhi-exchange:::" + card->objectName(),
                                               QVariant::fromValue((CardStar)card), Card::MethodNone);
                if (card_ex) {
                    CardMoveReason reason1(CardMoveReason::S_REASON_PUT, yueying->objectName(), "jizhi", QString());
                    CardMoveReason reason2(CardMoveReason::S_REASON_OVERRIDE, yueying->objectName(), "jizhi", QString());
                    CardsMoveStruct move1(card_ex->getEffectiveId(), yueying, NULL, Player::PlaceUnknown, Player::DrawPile, reason1);
                    CardsMoveStruct move2(ids, yueying, yueying, Player::PlaceUnknown, Player::PlaceHand, reason2);

                    QList<CardsMoveStruct> moves;
                    moves.append(move1);
                    moves.append(move2);
                    room->moveCardsAtomic(moves, false);
                } else {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, yueying->objectName(), "jizhi", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
        }

        return false;
    }
};

class Qicai: public TargetModSkill {
public:
    Qicai(): TargetModSkill("qicai") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Zhiheng: public ViewAsSkill {
public:
    Zhiheng(): ViewAsSkill("zhiheng") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Self->getMark("ZhihengInLatestKOF") > 0 && selected.length() >= 2) return false;
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        ZhihengCard *zhiheng_card = new ZhihengCard;
        zhiheng_card->addSubcards(cards);
        zhiheng_card->setSkillName(objectName());
        return zhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("ZhihengCard");
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@zhiheng";
    }
};

class ZhihengForKOF: public GameStartSkill {
public:
    ZhihengForKOF(): GameStartSkill("#zhiheng-for-kof") {
    }

    virtual void onGameStart(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical")
            room->setPlayerMark(player, "ZhihengInLatestKOF", 1);
    }
};

class Jiuyuan: public TriggerSkill {
public:
    Jiuyuan(): TriggerSkill("jiuyuan$") {
        events << TargetConfirmed << PreHpRecover;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("jiuyuan");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *sunquan, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach") && use.from && use.from->getKingdom() == "wu"
                && sunquan != use.from && sunquan->hasFlag("Global_Dying")) {
                room->setCardFlag(use.card, "jiuyuan");
            }
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct rec = data.value<RecoverStruct>();
            if (rec.card && rec.card->hasFlag("jiuyuan")) {
                if (sunquan->hasSkill("weidi"))
                    room->broadcastSkillInvoke("weidi");
                else
                    room->broadcastSkillInvoke("jiuyuan", rec.who->isMale() ? 1 : 2);

                room->notifySkillInvoked(sunquan, "jiuyuan");

                LogMessage log;
                log.type = "#JiuyuanExtraRecover";
                log.from = sunquan;
                log.to << rec.who;
                log.arg = objectName();
                room->sendLog(log);

                rec.recover++;
                data = QVariant::fromValue(rec);
            }
        }

        return false;
    }
};

class Yingzi: public DrawCardsSkill {
public:
    Yingzi(): DrawCardsSkill("yingzi") {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const{
        Room *room = zhouyu->getRoom();
        if (room->askForSkillInvoke(zhouyu, objectName())) {
            int index = qrand() % 2 + 1;
            if (!zhouyu->hasInnateSkill(objectName())) {
                if (zhouyu->hasSkill("mouduan"))
                    index += 2;
                else if (zhouyu->hasSkill("hunzi"))
                    index += 4;
            }

            room->broadcastSkillInvoke(objectName(), index);
            return n + 1;
        } else
            return n;
    }
};

class Fanjian: public ZeroCardViewAsSkill {
public:
    Fanjian(): ZeroCardViewAsSkill("fanjian") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("FanjianCard");
    }

    virtual const Card *viewAs() const{
        return new FanjianCard;
    }
};

class Keji: public TriggerSkill {
public:
    Keji(): TriggerSkill("keji") {
        events << PreCardUsed << CardResponded << EventPhaseChanging;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && TriggerSkill::triggerable(lvmeng)) {
                if (!lvmeng->hasFlag("KejiSlashInPlayPhase") && lvmeng->askForSkillInvoke(objectName())) {
                    if (lvmeng->getHandcardNum() > lvmeng->getMaxCards()) {
                        int index = qrand() % 2 + 1;
                        if (!lvmeng->hasInnateSkill(objectName()) && lvmeng->hasSkill("mouduan"))
                            index += 2;
                        room->broadcastSkillInvoke(objectName(), index);
                    }
                    lvmeng->skip(Player::Discard);
                }
            }
            if (lvmeng->hasFlag("KejiSlashInPlayPhase"))
                lvmeng->setFlags("-KejiSlashInPlayPhase");
        } else if (lvmeng->getPhase() == Player::Play) {
            CardStar card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;
            if (card->isKindOf("Slash"))
                lvmeng->setFlags("KejiSlashInPlayPhase");
        }

        return false;
    }
};

class Lianying: public TriggerSkill {
public:
    Lianying(): TriggerSkill("lianying") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            if (room->askForSkillInvoke(luxun, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                luxun->drawCards(1);
            }
        }
        return false;
    }
};

class LianyingForZeroMaxCards: public TriggerSkill {
public:
    LianyingForZeroMaxCards(): TriggerSkill("#lianying-for-zero-maxcards") {
        events << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.from == Player::Discard && player->hasFlag("LianyingZeroMaxCards")) {
            player->setFlags("-LianyingZeroMaxCards");
            if (player->isKongcheng() && room->askForSkillInvoke(player, "lianying")) {
                room->broadcastSkillInvoke("lianying");
                player->drawCards(1);
            }
        }
        return false;
    }
};

class Qixi: public OneCardViewAsSkill {
public:
    Qixi(): OneCardViewAsSkill("qixi") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->isBlack();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Dismantlement *dismantlement = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dismantlement->addSubcard(originalCard->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
    }
};

class Kurou: public ZeroCardViewAsSkill {
public:
    Kurou(): ZeroCardViewAsSkill("kurou") {
    }

    virtual const Card *viewAs() const{
        return new KurouCard;
    }
};

class Guose: public OneCardViewAsSkill {
public:
    Guose(): OneCardViewAsSkill("guose") {
        filter_pattern = ".|diamond";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
        indulgence->addSubcard(originalCard->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

class LiuliViewAsSkill: public OneCardViewAsSkill {
public:
    LiuliViewAsSkill(): OneCardViewAsSkill("liuli") {
        filter_pattern = ".!";
        response_pattern = "@@liuli";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LiuliCard *liuli_card = new LiuliCard;
        liuli_card->addSubcard(originalCard);
        return liuli_card;
    }
};

class Liuli: public TriggerSkill {
public:
    Liuli(): TriggerSkill("liuli") {
        events << TargetConfirming;
        view_as_skill = new LiuliViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isKindOf("Slash") && use.to.contains(daqiao) && daqiao->canDiscard(daqiao, "he")) {
            QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
            players.removeOne(use.from);

            bool can_invoke = false;
            foreach (ServerPlayer *p, players) {
                if (use.from->canSlash(p, use.card, false) && daqiao->inMyAttackRange(p)) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke) {
                QString prompt = "@liuli:" + use.from->objectName();
                room->setPlayerFlag(use.from, "LiuliSlashSource");
                // a temp nasty trick
                daqiao->tag["liuli-card"] = QVariant::fromValue((CardStar)use.card); // for the server (AI)
                room->setPlayerProperty(daqiao, "liuli", use.card->toString()); // for the client (UI)
                if (room->askForUseCard(daqiao, "@@liuli", prompt, -1, Card::MethodDiscard)) {
                    daqiao->tag.remove("liuli-card");
                    room->setPlayerProperty(daqiao, "liuli", QString());
                    room->setPlayerFlag(use.from, "-LiuliSlashSource");
                    foreach (ServerPlayer *p, players) {
                        if (p->hasFlag("LiuliTarget")) {
                            p->setFlags("-LiuliTarget");
                            use.to.removeOne(daqiao);
                            use.to.append(p);
                            room->sortByActionOrder(use.to);
                            data = QVariant::fromValue(use);
                            room->getThread()->trigger(TargetConfirming, room, p, data);
                            return false;
                        }
                    }
                } else {
                    daqiao->tag.remove("liuli-card");
                    room->setPlayerProperty(daqiao, "liuli", QString());
                    room->setPlayerFlag(use.from, "-LiuliSlashSource");
                }
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const{
        if (player->hasSkill("luoyan"))
            return qrand() % 2 + 3;
        else
            return qrand() % 2 + 1;
    }
};

class Jieyin: public ViewAsSkill {
public:
    Jieyin(): ViewAsSkill("jieyin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= 2 && !player->hasUsed("JieyinCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() > 1 || Self->isJilei(to_select))
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        JieyinCard *jieyin_card = new JieyinCard();
        jieyin_card->addSubcards(cards);
        return jieyin_card;
    }
};

class Xiaoji: public TriggerSkill {
public:
    Xiaoji(): TriggerSkill("xiaoji") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *sunshangxiang, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == sunshangxiang && move.from_places.contains(Player::PlaceEquip)) {
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (!sunshangxiang->isAlive())
                    return false;
                if (move.from_places[i] == Player::PlaceEquip) {
                    if (room->askForSkillInvoke(sunshangxiang, objectName())) {
                        room->broadcastSkillInvoke(objectName());
                        sunshangxiang->drawCards(2);
                    } else {
                        break;
                    }
                }
            }
        }
        return false;
    }
};

class Wushuang: public TriggerSkill {
public:
    Wushuang(): TriggerSkill("wushuang") {
        events << TargetConfirmed << CardFinished;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            bool can_invoke = false;
            if (use.card->isKindOf("Slash") && TriggerSkill::triggerable(use.from) && use.from == player) {
                can_invoke = true;
                QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
                for (int i = 0; i < use.to.length(); i++) {
                    if (jink_list.at(i).toInt() == 1)
                        jink_list.replace(i, QVariant(2));
                }
                player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
            }
            if (use.card->isKindOf("Duel")) {
                if (TriggerSkill::triggerable(use.from) && use.from == player)
                    can_invoke = true;
                if (TriggerSkill::triggerable(player) && use.to.contains(player))
                    can_invoke = true;
            }
            if (!can_invoke) return false;

            LogMessage log;
            log.from = player;
            log.arg = objectName();
            log.type = "#TriggerSkill";
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            room->broadcastSkillInvoke(objectName());
            if (use.card->isKindOf("Duel"))
                room->setPlayerMark(player, "WushuangTarget", 1);
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Duel")) {
                foreach (ServerPlayer *lvbu, room->getAllPlayers())
                    if (lvbu->getMark("WushuangTarget") > 0) room->setPlayerMark(lvbu, "WushuangTarget", 0);
            }
        }

        return false;
    }
};

class Lijian: public OneCardViewAsSkill {
public:
    Lijian(): OneCardViewAsSkill("lijian") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getAliveSiblings().length() > 1
               && player->canDiscard(player, "he") && !player->hasUsed("LijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LijianCard *lijian_card = new LijianCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

class Biyue: public PhaseChangeSkill {
public:
    Biyue(): PhaseChangeSkill("biyue") {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *diaochan) const{
        if (diaochan->getPhase() == Player::Finish) {
            Room *room = diaochan->getRoom();
            if (room->askForSkillInvoke(diaochan, objectName())) {
                room->broadcastSkillInvoke(objectName());
                diaochan->drawCards(1);
            }
        }

        return false;
    }
};

class Qingnang: public OneCardViewAsSkill {
public:
    Qingnang(): OneCardViewAsSkill("qingnang") {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "h") && !player->hasUsed("QingnangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        QingnangCard *qingnang_card = new QingnangCard;
        qingnang_card->addSubcard(originalCard->getId());
        return qingnang_card;
    }
};

class Jijiu: public OneCardViewAsSkill {
public:
    Jijiu(): OneCardViewAsSkill("jijiu") {
        filter_pattern = ".|red";
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("peach") && !player->hasFlag("Global_PreventPeach")
                && player->getPhase() == Player::NotActive && player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

class Qianxun: public ProhibitSkill {
public:
    Qianxun(): ProhibitSkill("qianxun") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("Snatch") || card->isKindOf("Indulgence"));
    }
};

class Mashu: public DistanceSkill {
public:
    Mashu(): DistanceSkill("mashu") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()))
            return -1;
        else
            return 0;
    }
};

class Mashu1v1: public TriggerSkill{
public:
    Mashu1v1(): TriggerSkill("#mashu-1v1-clear"){
        events << Debut;
    }

    virtual int getPriority() const{
        return 10;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return Config.GameMode == "02_1v1" && TriggerSkill::triggerable(target) && target->hasSkill("xiaoxi");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        room->handleAcquireDetachSkills(player, "-mashu");
        return false;
    }
};

class Wangzun: public PhaseChangeSkill {
public:
    Wangzun(): PhaseChangeSkill("wangzun") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (!isNormalGameMode(room->getMode()) || Config.EnableHegemony) //fix hegemony mode that yuanshu can trigger wangzun to wei players
            return false;
        if (target->isLord() && target->getPhase() == Player::Start) {
            ServerPlayer *yuanshu = room->findPlayerBySkillName(objectName());
            if (yuanshu && room->askForSkillInvoke(yuanshu, objectName())) {
                room->broadcastSkillInvoke(objectName());
                yuanshu->drawCards(1);
                room->setPlayerFlag(target, "WangzunDecMaxCards");
            }
        }
        return false;
    }
};

class WangzunMaxCards: public MaxCardsSkill {
public:
    WangzunMaxCards(): MaxCardsSkill("#wangzun-maxcard") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("WangzunDecMaxCards"))
            return -1;
        else
            return 0;
    }
};

class Tongji: public ProhibitSkill {
public:
    Tongji(): ProhibitSkill("tongji") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (card->isKindOf("Slash")) {
            // get rangefix
            int rangefix = 0;
            if (card->isVirtualCard()) {
                QList<int> subcards = card->getSubcards();
                if (from->getWeapon() && subcards.contains(from->getWeapon()->getId())) {
                    const Weapon *weapon = qobject_cast<const Weapon *>(from->getWeapon()->getRealCard());
                    rangefix += weapon->getRange() - from->getAttackRange(false);
                }

                if (from->getOffensiveHorse() && subcards.contains(from->getOffensiveHorse()->getId()))
                    rangefix += 1;
            }
            // find yuanshu
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->hasSkill(objectName()) && p != to && p->getHandcardNum() > p->getHp()
                    && from->distanceTo(p, rangefix) <= from->getAttackRange()) {
                    return true;
                }
            }
        }
        return false;
    }
};

class Yaowu: public TriggerSkill {
public:
    Yaowu(): TriggerSkill("yaowu") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()
            && damage.from && damage.from->isAlive()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(damage.to, objectName());

            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = damage.to;
            log.arg = objectName();
            room->sendLog(log);

            if (damage.from->isWounded() && room->askForChoice(damage.from, objectName(), "recover+draw", data) == "recover") {
                RecoverStruct recover;
                recover.who = damage.to;
                room->recover(damage.from, recover);
            } else {
                damage.from->drawCards(1);
            }
        }
        return false;
    }
};

void StandardPackage::addGenerals() {
    // Wei
    General *caocao = new General(this, "caocao$", "wei"); // WEI 001
    caocao->addSkill(new Jianxiong);
    caocao->addSkill(new Hujia);

    General *simayi = new General(this, "simayi", "wei", 3); // WEI 002
    simayi->addSkill(new Fankui);
    simayi->addSkill(new Guicai);

    General *xiahoudun = new General(this, "xiahoudun", "wei"); // WEI 003
    xiahoudun->addSkill(new Ganglie);

    General *zhangliao = new General(this, "zhangliao", "wei"); // WEI 004
    zhangliao->addSkill(new Tuxi);

    General *xuchu = new General(this, "xuchu", "wei"); // WEI 005
    xuchu->addSkill(new Luoyi);
    xuchu->addSkill(new LuoyiBuff);
    related_skills.insertMulti("luoyi", "#luoyi");

    General *guojia = new General(this, "guojia", "wei", 3); // WEI 006
    guojia->addSkill(new Tiandu);
    guojia->addSkill(new Yiji);

    General *zhenji = new General(this, "zhenji", "wei", 3, false); // WEI 007
    zhenji->addSkill(new Qingguo);
    zhenji->addSkill(new Luoshen);

    // Shu
    General *liubei = new General(this, "liubei$", "shu"); // SHU 001
    liubei->addSkill(new Rende);
    liubei->addSkill(new Jijiang);

    General *guanyu = new General(this, "guanyu", "shu"); // SHU 002
    guanyu->addSkill(new Wusheng);

    General *zhangfei = new General(this, "zhangfei", "shu"); // SHU 003
    zhangfei->addSkill(new Paoxiao);

    General *zhugeliang = new General(this, "zhugeliang", "shu", 3); // SHU 004
    zhugeliang->addSkill(new Guanxing);
    zhugeliang->addSkill(new Kongcheng);
    zhugeliang->addSkill(new KongchengEffect);
    related_skills.insertMulti("kongcheng", "#kongcheng-effect");

    General *zhaoyun = new General(this, "zhaoyun", "shu"); // SHU 005
    zhaoyun->addSkill(new Longdan);

    General *machao = new General(this, "machao", "shu"); // SHU 006
    machao->addSkill(new Tieji);
    machao->addSkill(new Mashu);
    machao->addSkill(new Mashu1v1);
    related_skills.insertMulti("mashu", "#mashu-1v1-clear");

    General *huangyueying = new General(this, "huangyueying", "shu", 3, false); // SHU 007
    huangyueying->addSkill(new Jizhi);
    huangyueying->addSkill(new Qicai);

    // Wu
    General *sunquan = new General(this, "sunquan$", "wu"); // WU 001
    sunquan->addSkill(new Zhiheng);
    sunquan->addSkill(new ZhihengForKOF);
    sunquan->addSkill(new Jiuyuan);
    related_skills.insertMulti("zhiheng", "#zhiheng-for-kof");

    General *ganning = new General(this, "ganning", "wu"); // WU 002
    ganning->addSkill(new Qixi);

    General *lvmeng = new General(this, "lvmeng", "wu"); // WU 003
    lvmeng->addSkill(new Keji);

    General *huanggai = new General(this, "huanggai", "wu"); // WU 004
    huanggai->addSkill(new Kurou);

    General *zhouyu = new General(this, "zhouyu", "wu", 3); // WU 005
    zhouyu->addSkill(new Yingzi);
    zhouyu->addSkill(new Fanjian);

    General *daqiao = new General(this, "daqiao", "wu", 3, false); // WU 006
    daqiao->addSkill(new Guose);
    daqiao->addSkill(new Liuli);

    General *luxun = new General(this, "luxun", "wu", 3); // WU 007
    luxun->addSkill(new Qianxun);
    luxun->addSkill(new Lianying);
    luxun->addSkill(new LianyingForZeroMaxCards);
    related_skills.insertMulti("lianying", "#lianying-for-zero-maxcards");

    General *sunshangxiang = new General(this, "sunshangxiang", "wu", 3, false); // WU 008
    sunshangxiang->addSkill(new Jieyin);
    sunshangxiang->addSkill(new Xiaoji);

    // Qun
    General *huatuo = new General(this, "huatuo", "qun", 3); // QUN 001
    huatuo->addSkill(new Qingnang);
    huatuo->addSkill(new Jijiu);

    General *lvbu = new General(this, "lvbu", "qun"); // QUN 002
    lvbu->addSkill(new Wushuang);

    General *diaochan = new General(this, "diaochan", "qun", 3, false); // QUN 003
    diaochan->addSkill(new Lijian);
    diaochan->addSkill(new Biyue);

    General *st_yuanshu = new General(this, "st_yuanshu", "qun", 4);
    st_yuanshu->addSkill(new Wangzun);
    st_yuanshu->addSkill(new WangzunMaxCards);
    related_skills.insertMulti("wangzun", "#wangzun-maxcard");
    st_yuanshu->addSkill(new Tongji);

    General *st_huaxiong = new General(this, "st_huaxiong", "qun", 6);
    st_huaxiong->addSkill(new Yaowu);

    // for skill cards
    addMetaObject<ZhihengCard>();
    addMetaObject<RendeCard>();
    addMetaObject<TuxiCard>();
    addMetaObject<JieyinCard>();
    addMetaObject<KurouCard>();
    addMetaObject<LijianCard>();
    addMetaObject<FanjianCard>();
    addMetaObject<QingnangCard>();
    addMetaObject<LiuliCard>();
    addMetaObject<JijiangCard>();
}

class SuperZhiheng: public Zhiheng {
public:
    SuperZhiheng():Zhiheng() {
        setObjectName("super_zhiheng");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && player->usedTimes("ZhihengCard") < (player->getLostHp() + 1);
    }
};

class SuperGuanxing: public Guanxing {
public:
    SuperGuanxing(): Guanxing() {
        setObjectName("super_guanxing");
    }

    virtual int getGuanxingNum(Room *room) const{
        return 5;
    }
};

class SuperMaxCards: public MaxCardsSkill {
public:
    SuperMaxCards(): MaxCardsSkill("super_max_cards") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return target->getMark("@max_cards_test");
        return 0;
    }
};

class SuperOffensiveDistance: public DistanceSkill {
public:
    SuperOffensiveDistance(): DistanceSkill("super_offensive_distance") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()))
            return -from->getMark("@offensive_distance_test");
        else
            return 0;
    }
};

class SuperDefensiveDistance: public DistanceSkill {
public:
    SuperDefensiveDistance(): DistanceSkill("super_defensive_distance") {
    }

    virtual int getCorrect(const Player *, const Player *to) const{
        if (to->hasSkill(objectName()))
            return to->getMark("@defensive_distance_test");
        else
            return 0;
    }
};

#include "sp-package.h"
class SuperYongsi: public Yongsi {
public:
    SuperYongsi(): Yongsi() {
        setObjectName("super_yongsi");
    }

    virtual int getKingdoms(ServerPlayer *yuanshu) const{
        return yuanshu->getMark("@yongsi_test");
    }
};

#include "wind.h"
class SuperJushou: public Jushou {
public:
    SuperJushou(): Jushou() {
        setObjectName("super_jushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *caoren) const{
        return caoren->getMark("@jushou_test");
    }
};

#include "god.h"
#include "maneuvering.h"
class NosJuejing: public TriggerSkill {
public:
    NosJuejing(): TriggerSkill("nosjuejing") {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != gaodayihao && move.to != gaodayihao)
                return false;
            if ((move.to_place != Player::PlaceHand && !move.from_places.contains(Player::PlaceHand))
                || gaodayihao->getPhase() == Player::Discard) {
                return false;
            }
        }
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Draw) {
                gaodayihao->skip(change.to);
                return false;
            } else if (change.to != Player::Finish)
                return false;
        }
        if (gaodayihao->getHandcardNum() == 4)
            return false;
        int diff = abs(gaodayihao->getHandcardNum() - 4);
        if (gaodayihao->getHandcardNum() < 4) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(gaodayihao, objectName());
            gaodayihao->drawCards(diff);
        } else if (gaodayihao->getHandcardNum() > 4) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(gaodayihao, objectName());
            room->askForDiscard(gaodayihao, objectName(), diff, diff);
        }

        return false;
    }
};

class NosLonghun: public Longhun {
public:
    NosLonghun(): Longhun() {
        setObjectName("noslonghun");
    }

    virtual int getEffHp(const Player *) const{
        return 1;
    }
};

class NosDuojian: public TriggerSkill {
public:
    NosDuojian(): TriggerSkill("#noslonghun_duojian") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &) const{
        if (gaodayihao->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->getOtherPlayers(gaodayihao)) {
               if (p->getWeapon() && p->getWeapon()->isKindOf("QinggangSword")) {
                   if (room->askForSkillInvoke(gaodayihao, "noslonghun")) {
                       room->broadcastSkillInvoke("noslonghun", 5);
                       gaodayihao->obtainCard(p->getWeapon());
                    }
                    break;
                }
            }
        }

        return false;
    }
};

class Xianiao: public TriggerSkill{
public:
    Xianiao(): TriggerSkill("xianiao"){
        events << Damage;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *xiahoujie = room->findPlayerBySkillName(objectName());
        if (xiahoujie == NULL || xiahoujie->isDead() || !player->inMyAttackRange(xiahoujie))
            return false;

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(xiahoujie, objectName());

        xiahoujie->throwAllHandCards();
        xiahoujie->drawCards(player->getHp());

        return false;
    }
};

class Tangqiang: public TriggerSkill{
public:
    Tangqiang(): TriggerSkill("tangqiang"){
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (player == death.who && death.damage && death.damage->from){
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            room->loseMaxHp(death.damage->from, 1);
            room->acquireSkill(death.damage->from, objectName());
        }
        return false;
    }
};


class NosZhenggong: public MasochismSkill {
public:
    NosZhenggong(): MasochismSkill("noszhenggong") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("nosbaijiang") == 0;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        if (damage.from && damage.from->hasEquip()) {
            QVariant data = QVariant::fromValue((PlayerStar)damage.from);
            if (!zhonghui->askForSkillInvoke(objectName(), data))
                return;

            Room *room = zhonghui->getRoom();
            room->broadcastSkillInvoke(objectName());
            int equip = room->askForCardChosen(zhonghui, damage.from, "e", objectName());
            const Card *card = Sanguosha->getCard(equip);

            int equip_index = -1;
            const EquipCard *equipcard = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equipcard->location());

            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct move1(equip, zhonghui, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_ROB, zhonghui->objectName()));
            exchangeMove.push_back(move1);
            if (zhonghui->getEquip(equip_index) != NULL) {
                CardsMoveStruct move2(zhonghui->getEquip(equip_index)->getId(), NULL, Player::DiscardPile,
                    CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, zhonghui->objectName()));
                exchangeMove.push_back(move2);
            }
            room->moveCardsAtomic(exchangeMove, true);
        }
    }
};

class NosQuanji: public TriggerSkill {
public:
    NosQuanji(): TriggerSkill("nosquanji") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        foreach (ServerPlayer *zhonghui, room->findPlayersBySkillName(objectName())) {
            if (zhonghui == player || zhonghui->isKongcheng()
                || zhonghui->getMark("nosbaijiang") > 0 || player->isKongcheng())
                continue;

            if (room->askForSkillInvoke(zhonghui, "nosquanji")) {
                room->broadcastSkillInvoke(objectName(), 1);
                if (zhonghui->pindian(player, objectName(), NULL)) {
                    if (!skip) {
                        room->broadcastSkillInvoke(objectName(), 2);
                        player->skip(Player::Start);
                        player->skip(Player::Judge);
                        skip = true;
                    } else {
                        room->broadcastSkillInvoke(objectName(), 3);
                    }
                }
            }
        }
        return skip;
    }
};

class NosBaijiang: public PhaseChangeSkill {
public:
    NosBaijiang(): PhaseChangeSkill("nosbaijiang") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getMark("nosbaijiang") == 0
            && target->getPhase() == Player::Start
            && target->getEquips().length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosBaijiangWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getEquips().length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosBaijiangAnimate", 5000);
        room->addPlayerMark(zhonghui, "nosbaijiang");

        if (room->changeMaxHpForAwakenSkill(zhonghui, 1)) {
            RecoverStruct recover;
            recover.who = zhonghui;
            room->recover(zhonghui, recover);
            room->handleAcquireDetachSkills(zhonghui, "-noszhenggong|-nosquanji|nosyexin");
        }

        return false;
    }
};

class NosYexinViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosYexinViewAsSkill(): ZeroCardViewAsSkill("nosyexin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("nospower").isEmpty() && !player->hasUsed("NosYexinCard");
    }

    virtual const Card *viewAs() const{
        return new NosYexinCard;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

class NosYexin: public TriggerSkill {
public:
    NosYexin(): TriggerSkill("nosyexin") {
        events << Damage << Damaged;
        view_as_skill = new NosYexinViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhonghui, QVariant &) const{
        if (!zhonghui->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName(), 1);
        zhonghui->addToPile("nospower", room->drawCard());

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 2;
    }
};

class NosPaiyi: public PhaseChangeSkill {
public:
    NosPaiyi(): PhaseChangeSkill("nospaiyi") {
        _m_place["Judging"] = Player::PlaceDelayedTrick;
        _m_place["Equip"] = Player::PlaceEquip;
        _m_place["Hand"] = Player::PlaceHand;
    }

    QString getPlace(Room *room, ServerPlayer *player, QStringList places) const{
        if (places.length() > 0) {
            QString place = room->askForChoice(player, "nospaiyi", places.join("+"));
            return place;
        }
        return QString();
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        if (zhonghui->getPhase() != Player::Finish || zhonghui->getPile("nospower").isEmpty())
            return false;

        Room *room = zhonghui->getRoom();
        QList<int> powers = zhonghui->getPile("nospower");
        if (powers.isEmpty() || !room->askForSkillInvoke(zhonghui, objectName()))
            return false;
        QStringList places;
        places << "Hand";

        room->fillAG(powers, zhonghui);
        int power = room->askForAG(zhonghui, powers, false, "nospaiyi");
        room->clearAG(zhonghui);

        if (power == -1)
            power = powers.first();

        const Card *card = Sanguosha->getCard(power);

        ServerPlayer *target = room->askForPlayerChosen(zhonghui, room->getAlivePlayers(), "nospaiyi", "@nospaiyi-to:::" + card->objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, zhonghui->objectName(), "nospaiyi", QString());

        if (card->isKindOf("DelayedTrick")) {
            if (!zhonghui->isProhibited(target, card) && !target->containsTrick(card->objectName()))
                places << "Judging";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else if (card->isKindOf("EquipCard")) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            if (!target->getEquip(equip->location()))
                places << "Equip";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);

        int index = 1;
        if (target != zhonghui) {
            index++;
            room->drawCards(zhonghui, 1);
        }
        room->broadcastSkillInvoke(objectName(), index);

        return false;
    }

private:
    QMap<QString, Player::Place> _m_place;
};

class NosZili: public PhaseChangeSkill {
public:
    NosZili(): PhaseChangeSkill("noszili") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getMark("noszili") == 0
            && target->getPhase() == Player::Start
            && target->getPile("nospower").length() >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("nospower").length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosZiliAnimate", 5000);

        room->addPlayerMark(zhonghui, "noszili");
        if (room->changeMaxHpForAwakenSkill(zhonghui))
            room->acquireSkill(zhonghui, "nospaiyi");

        return false;
    }
};


TestPackage::TestPackage()
    : Package("test")
{
    // for test only
    General *zhiba_sunquan = new General(this, "zhiba_sunquan$", "wu", 4, true, true);
    zhiba_sunquan->addSkill(new SuperZhiheng);
    zhiba_sunquan->addSkill("jiuyuan");

    General *wuxing_zhuge = new General(this, "wuxing_zhugeliang", "shu", 3, true, true);
    wuxing_zhuge->addSkill(new SuperGuanxing);
    wuxing_zhuge->addSkill("kongcheng");

    General *super_yuanshu = new General(this, "super_yuanshu", "qun", 4, true, true, true);
    super_yuanshu->addSkill(new SuperYongsi);
    super_yuanshu->addSkill(new MarkAssignSkill("@yongsi_test", 4));
    related_skills.insertMulti("super_yongsi", "#@yongsi_test-4");
    super_yuanshu->addSkill("weidi");

    General *super_caoren = new General(this, "super_caoren", "wei", 4, true, true, true);
    super_caoren->addSkill(new SuperJushou);
    super_caoren->addSkill(new MarkAssignSkill("@jushou_test", 5));
    related_skills.insertMulti("super_jushou", "#@jushou_test-5");

    General *gd_shenzhaoyun = new General(this, "gaodayihao", "god", 1, true, true);
    gd_shenzhaoyun->addSkill(new NosJuejing);
    gd_shenzhaoyun->addSkill(new NosLonghun);
    gd_shenzhaoyun->addSkill(new NosDuojian);
    related_skills.insertMulti("noslonghun", "#noslonghun_duojian");

    General *nobenghuai_dongzhuo = new General(this, "nobenghuai_dongzhuo$", "qun", 4, true, true, true);
    nobenghuai_dongzhuo->addSkill("jiuchi");
    nobenghuai_dongzhuo->addSkill("roulin");
    nobenghuai_dongzhuo->addSkill("baonue");


    new General(this, "sujiang", "god", 5, true, true);
    new General(this, "sujiangf", "god", 5, false, true);

    new General(this, "anjiang", "god", 4, true, true, true);

    General *xiahoujie = new General(this, "xiahoujie", "wei", 3, true, true, true);
    xiahoujie->addSkill(new Xianiao);
    xiahoujie->addSkill(new Tangqiang);

    General *zhonghui = new General(this, "nos_zhonghui", "wei", 3, true, true, true);
    zhonghui->addSkill(new NosZhenggong);
    zhonghui->addSkill(new NosQuanji);
    zhonghui->addSkill(new NosBaijiang);
    zhonghui->addSkill(new NosZili);
    zhonghui->addRelateSkill("nosyexin");
    zhonghui->addRelateSkill("#nosyexin-fake-move");
    zhonghui->addRelateSkill("nospaiyi");
    related_skills.insertMulti("nosyexin", "#nosyexin-fake-move");

    addMetaObject<NosYexinCard>();

    skills << new SuperMaxCards << new SuperOffensiveDistance << new SuperDefensiveDistance <<
        new NosYexin << new FakeMoveSkill("nosyexin") << new NosPaiyi; //for zhonghui

}

ADD_PACKAGE(Test)

