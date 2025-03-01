/*
 * Copyright (c) 2021-2023, Linus Groh <linusg@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Time.h>
#include <AK/TypeCasts.h>
#include <LibCrypto/BigInt/UnsignedBigInteger.h>
#include <LibJS/Runtime/AbstractOperations.h>
#include <LibJS/Runtime/Date.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/IteratorOperations.h>
#include <LibJS/Runtime/Temporal/AbstractOperations.h>
#include <LibJS/Runtime/Temporal/Calendar.h>
#include <LibJS/Runtime/Temporal/Instant.h>
#include <LibJS/Runtime/Temporal/PlainDateTime.h>
#include <LibJS/Runtime/Temporal/TimeZone.h>
#include <LibJS/Runtime/Temporal/TimeZoneConstructor.h>
#include <LibJS/Runtime/Temporal/ZonedDateTime.h>
#include <LibTimeZone/TimeZone.h>

namespace JS::Temporal {

// 11 Temporal.TimeZone Objects, https://tc39.es/proposal-temporal/#sec-temporal-timezone-objects
TimeZone::TimeZone(Object& prototype)
    : Object(ConstructWithPrototypeTag::Tag, prototype)
{
}

// 11.1.1 IsAvailableTimeZoneName ( timeZone ), https://tc39.es/proposal-temporal/#sec-isavailabletimezonename
bool is_available_time_zone_name(StringView time_zone)
{
    // 1. Let timeZones be AvailableTimeZones().
    // 2. For each String candidate in timeZones, do
    //     a. If timeZone is an ASCII-case-insensitive match for candidate, return true.
    // 3. Return false.
    // NOTE: When LibTimeZone is built without ENABLE_TIME_ZONE_DATA, this only recognizes 'UTC',
    // which matches the minimum requirements of the Temporal spec.
    return ::TimeZone::time_zone_from_string(time_zone).has_value();
}

// 6.4.2 CanonicalizeTimeZoneName ( timeZone ), https://tc39.es/ecma402/#sec-canonicalizetimezonename
// 11.1.2 CanonicalizeTimeZoneName ( timeZone ), https://tc39.es/proposal-temporal/#sec-canonicalizetimezonename
// 15.1.2 CanonicalizeTimeZoneName ( timeZone ), https://tc39.es/proposal-temporal/#sup-canonicalizetimezonename
ThrowCompletionOr<String> canonicalize_time_zone_name(VM& vm, StringView time_zone)
{
    // 1. Let ianaTimeZone be the String value of the Zone or Link name of the IANA Time Zone Database that is an ASCII-case-insensitive match of timeZone as described in 6.1.
    // 2. If ianaTimeZone is a Link name, let ianaTimeZone be the String value of the corresponding Zone name as specified in the file backward of the IANA Time Zone Database.
    auto iana_time_zone = ::TimeZone::canonicalize_time_zone(time_zone);

    // 3. If ianaTimeZone is one of "Etc/UTC", "Etc/GMT", or "GMT", return "UTC".
    // NOTE: This is already done in canonicalize_time_zone().

    // 4. Return ianaTimeZone.
    return TRY_OR_THROW_OOM(vm, String::from_utf8(*iana_time_zone));
}

// 11.6.1 CreateTemporalTimeZone ( identifier [ , newTarget ] ), https://tc39.es/proposal-temporal/#sec-temporal-createtemporaltimezone
ThrowCompletionOr<TimeZone*> create_temporal_time_zone(VM& vm, StringView identifier, FunctionObject const* new_target)
{
    auto& realm = *vm.current_realm();

    // 1. If newTarget is not present, set newTarget to %Temporal.TimeZone%.
    if (!new_target)
        new_target = realm.intrinsics().temporal_time_zone_constructor();

    // 2. Let object be ? OrdinaryCreateFromConstructor(newTarget, "%Temporal.TimeZone.prototype%", « [[InitializedTemporalTimeZone]], [[Identifier]], [[OffsetNanoseconds]] »).
    auto object = TRY(ordinary_create_from_constructor<TimeZone>(vm, *new_target, &Intrinsics::temporal_time_zone_prototype));

    // 3. If IsTimeZoneOffsetString(identifier) is true, then
    if (is_time_zone_offset_string(identifier)) {
        // a. Let offsetNanosecondsResult be ParseTimeZoneOffsetString(identifier).
        auto offset_nanoseconds_result = parse_time_zone_offset_string(identifier);

        // b. Set object.[[Identifier]] to ! FormatTimeZoneOffsetString(offsetNanosecondsResult).
        object->set_identifier(MUST_OR_THROW_OOM(format_time_zone_offset_string(vm, offset_nanoseconds_result)));

        // c. Set object.[[OffsetNanoseconds]] to offsetNanosecondsResult.
        object->set_offset_nanoseconds(offset_nanoseconds_result);
    }
    // 4. Else,
    else {
        // a. Assert: ! CanonicalizeTimeZoneName(identifier) is identifier.
        VERIFY(MUST_OR_THROW_OOM(canonicalize_time_zone_name(vm, identifier)) == identifier);

        // b. Set object.[[Identifier]] to identifier.
        object->set_identifier(TRY_OR_THROW_OOM(vm, String::from_utf8(identifier)));

        // c. Set object.[[OffsetNanoseconds]] to undefined.
        // NOTE: No-op.
    }

    // 5. Return object.
    return object.ptr();
}

// 11.6.2 GetISOPartsFromEpoch ( epochNanoseconds ), https://tc39.es/proposal-temporal/#sec-temporal-getisopartsfromepoch
ISODateTime get_iso_parts_from_epoch(VM& vm, Crypto::SignedBigInteger const& epoch_nanoseconds)
{
    // 1. Assert: ! IsValidEpochNanoseconds(ℤ(epochNanoseconds)) is true.
    VERIFY(is_valid_epoch_nanoseconds(BigInt::create(vm, epoch_nanoseconds)));

    // 2. Let remainderNs be epochNanoseconds modulo 10^6.
    auto remainder_ns_bigint = modulo(epoch_nanoseconds, Crypto::UnsignedBigInteger { 1'000'000 });
    auto remainder_ns = remainder_ns_bigint.to_double();

    // 3. Let epochMilliseconds be 𝔽((epochNanoseconds - remainderNs) / 10^6).
    auto epoch_milliseconds_bigint = epoch_nanoseconds.minus(remainder_ns_bigint).divided_by(Crypto::UnsignedBigInteger { 1'000'000 }).quotient;
    auto epoch_milliseconds = epoch_milliseconds_bigint.to_double();

    // 4. Let year be ℝ(! YearFromTime(epochMilliseconds)).
    auto year = year_from_time(epoch_milliseconds);

    // 5. Let month be ℝ(! MonthFromTime(epochMilliseconds)) + 1.
    auto month = static_cast<u8>(month_from_time(epoch_milliseconds) + 1);

    // 6. Let day be ℝ(! DateFromTime(epochMilliseconds)).
    auto day = date_from_time(epoch_milliseconds);

    // 7. Let hour be ℝ(! HourFromTime(epochMilliseconds)).
    auto hour = hour_from_time(epoch_milliseconds);

    // 8. Let minute be ℝ(! MinFromTime(epochMilliseconds)).
    auto minute = min_from_time(epoch_milliseconds);

    // 9. Let second be ℝ(! SecFromTime(epochMilliseconds)).
    auto second = sec_from_time(epoch_milliseconds);

    // 10. Let millisecond be ℝ(! msFromTime(epochMilliseconds)).
    auto millisecond = ms_from_time(epoch_milliseconds);

    // 11. Let microsecond be floor(remainderNs / 1000).
    auto microsecond = floor(remainder_ns / 1000);

    // 12. Assert: microsecond < 1000.
    VERIFY(microsecond < 1000);

    // 13. Let nanosecond be remainderNs modulo 1000.
    auto nanosecond = modulo(remainder_ns, 1000);

    // 14. Return the Record { [[Year]]: year, [[Month]]: month, [[Day]]: day, [[Hour]]: hour, [[Minute]]: minute, [[Second]]: second, [[Millisecond]]: millisecond, [[Microsecond]]: microsecond, [[Nanosecond]]: nanosecond }.
    return { .year = year, .month = month, .day = day, .hour = hour, .minute = minute, .second = second, .millisecond = millisecond, .microsecond = static_cast<u16>(microsecond), .nanosecond = static_cast<u16>(nanosecond) };
}

// 11.6.3 GetNamedTimeZoneNextTransition ( timeZoneIdentifier, epochNanoseconds ), https://tc39.es/proposal-temporal/#sec-temporal-getianatimezonenexttransition
BigInt* get_named_time_zone_next_transition(VM&, [[maybe_unused]] StringView time_zone_identifier, [[maybe_unused]] BigInt const& epoch_nanoseconds)
{
    // The implementation-defined abstract operation GetNamedTimeZoneNextTransition takes arguments timeZoneIdentifier (a String) and epochNanoseconds (a BigInt) and returns a BigInt or null.
    // The returned value t represents the number of nanoseconds since the Unix epoch in UTC that corresponds to the first time zone transition after epochNanoseconds in the IANA time zone identified by timeZoneIdentifier. The operation returns null if no such transition exists for which t ≤ ℤ(nsMaxInstant).
    // Given the same values of epochNanoseconds and timeZoneIdentifier, the result must be the same for the lifetime of the surrounding agent.

    // TODO: Implement this
    return nullptr;
}

// 11.6.4 GetNamedTimeZonePreviousTransition ( timeZoneIdentifier, epochNanoseconds ), https://tc39.es/proposal-temporal/#sec-temporal-getianatimezoneprevioustransition
BigInt* get_named_time_zone_previous_transition(VM&, [[maybe_unused]] StringView time_zone_identifier, [[maybe_unused]] BigInt const& epoch_nanoseconds)
{
    // The implementation-defined abstract operation GetNamedTimeZonePreviousTransition takes arguments timeZoneIdentifier (a String) and epochNanoseconds (a BigInt) and returns a BigInt or null.
    // The returned value t represents the number of nanoseconds since the Unix epoch in UTC that corresponds to the last time zone transition before epochNanoseconds in the IANA time zone identified by timeZoneIdentifier. The operation returns null if no such transition exists for which t ≥ ℤ(nsMinInstant).
    // Given the same values of epochNanoseconds and timeZoneIdentifier, the result must be the same for the lifetime of the surrounding agent.

    // TODO: Implement this
    return nullptr;
}

// 11.6.5 FormatTimeZoneOffsetString ( offsetNanoseconds ), https://tc39.es/proposal-temporal/#sec-temporal-formattimezoneoffsetstring
ThrowCompletionOr<String> format_time_zone_offset_string(VM& vm, double offset_nanoseconds)
{
    auto offset = static_cast<i64>(offset_nanoseconds);

    // 1. Assert: offsetNanoseconds is an integer.
    VERIFY(offset == offset_nanoseconds);

    StringBuilder builder;
    // 2. If offsetNanoseconds ≥ 0, let sign be "+"; otherwise, let sign be "-".
    if (offset >= 0)
        builder.append('+');
    else
        builder.append('-');

    // 3. Let _offsetNanoseconds_ be abs(_offsetNanoseconds_).
    offset = AK::abs(offset);

    // 4. Let nanoseconds be offsetNanoseconds modulo 10^9.
    auto nanoseconds = offset % 1000000000;

    // 5. Let seconds be floor(offsetNanoseconds / 10^9) modulo 60.
    auto seconds = (offset / 1000000000) % 60;
    // 6. Let minutes be floor(offsetNanoseconds / (6 × 10^10)) modulo 60.
    auto minutes = (offset / 60000000000) % 60;
    // 7. Let hours be floor(offsetNanoseconds / (3.6 × 10^12)).
    auto hours = offset / 3600000000000;

    // 8. Let h be ToZeroPaddedDecimalString(hours, 2).
    builder.appendff("{:02}", hours);
    // 9. Let m be ToZeroPaddedDecimalString(minutes, 2).
    builder.appendff(":{:02}", minutes);
    // 10. Let s be ToZeroPaddedDecimalString(seconds, 2).
    // NOTE: Handled by steps 11 & 12

    // 11. If nanoseconds ≠ 0, then
    if (nanoseconds != 0) {
        // a. Let fraction be ToZeroPaddedDecimalString(nanoseconds, 9).
        auto fraction = TRY_OR_THROW_OOM(vm, String::formatted("{:09}", nanoseconds));

        // b. Set fraction to the longest possible substring of fraction starting at position 0 and not ending with the code unit 0x0030 (DIGIT ZERO).
        fraction = TRY_OR_THROW_OOM(vm, fraction.trim("0"sv, TrimMode::Right));

        // c. Let post be the string-concatenation of the code unit 0x003A (COLON), s, the code unit 0x002E (FULL STOP), and fraction.
        builder.appendff(":{:02}.{}", seconds, fraction);
    }
    // 12. Else if seconds ≠ 0, then
    else if (seconds != 0) {
        // a. Let post be the string-concatenation of the code unit 0x003A (COLON) and s.
        builder.appendff(":{:02}", seconds);
    }
    // 13. Else,
    //    a. Let post be the empty String.

    // 14. Return the string-concatenation of sign, h, the code unit 0x003A (COLON), m, and post.
    return TRY_OR_THROW_OOM(vm, builder.to_string());
}

// 11.6.6 FormatISOTimeZoneOffsetString ( offsetNanoseconds ), https://tc39.es/proposal-temporal/#sec-temporal-formatisotimezoneoffsetstring
ThrowCompletionOr<String> format_iso_time_zone_offset_string(VM& vm, double offset_nanoseconds)
{
    // 1. Assert: offsetNanoseconds is an integer.
    VERIFY(trunc(offset_nanoseconds) == offset_nanoseconds);

    // 2. Set offsetNanoseconds to RoundNumberToIncrement(offsetNanoseconds, 60 × 10^9, "halfExpand").
    offset_nanoseconds = round_number_to_increment(offset_nanoseconds, 60000000000, "halfExpand"sv);

    // 3. If offsetNanoseconds ≥ 0, let sign be "+"; otherwise, let sign be "-".
    auto sign = offset_nanoseconds >= 0 ? "+"sv : "-"sv;

    // 4. Set offsetNanoseconds to abs(offsetNanoseconds).
    offset_nanoseconds = fabs(offset_nanoseconds);

    // 5. Let minutes be offsetNanoseconds / (60 × 10^9) modulo 60.
    auto minutes = fmod(offset_nanoseconds / 60000000000, 60);

    // 6. Let hours be floor(offsetNanoseconds / (3600 × 10^9)).
    auto hours = floor(offset_nanoseconds / 3600000000000);

    // 7. Let h be ToZeroPaddedDecimalString(hours, 2).
    // 8. Let m be ToZeroPaddedDecimalString(minutes, 2).
    // 9. Return the string-concatenation of sign, h, the code unit 0x003A (COLON), and m.
    return TRY_OR_THROW_OOM(vm, String::formatted("{}{:02}:{:02}", sign, (u32)hours, (u32)minutes));
}

// 11.6.7 ToTemporalTimeZone ( temporalTimeZoneLike ), https://tc39.es/proposal-temporal/#sec-temporal-totemporaltimezone
ThrowCompletionOr<Object*> to_temporal_time_zone(VM& vm, Value temporal_time_zone_like)
{
    // 1. If Type(temporalTimeZoneLike) is Object, then
    if (temporal_time_zone_like.is_object()) {
        // a. If temporalTimeZoneLike has an [[InitializedTemporalTimeZone]] internal slot, then
        if (is<TimeZone>(temporal_time_zone_like.as_object())) {
            // i. Return temporalTimeZoneLike.
            return &temporal_time_zone_like.as_object();
        }

        // b. If temporalTimeZoneLike has an [[InitializedTemporalZonedDateTime]] internal slot, then
        if (is<ZonedDateTime>(temporal_time_zone_like.as_object())) {
            auto& zoned_date_time = static_cast<ZonedDateTime&>(temporal_time_zone_like.as_object());

            // i. Return temporalTimeZoneLike.[[TimeZone]].
            return &zoned_date_time.time_zone();
        }

        // c. If temporalTimeZoneLike has an [[InitializedTemporalCalendar]] internal slot, throw a RangeError exception.
        if (is<Calendar>(temporal_time_zone_like.as_object()))
            return vm.throw_completion<RangeError>(ErrorType::TemporalUnexpectedCalendarObject);

        // d. If ? HasProperty(temporalTimeZoneLike, "timeZone") is false, return temporalTimeZoneLike.
        if (!TRY(temporal_time_zone_like.as_object().has_property(vm.names.timeZone)))
            return &temporal_time_zone_like.as_object();

        // e. Set temporalTimeZoneLike to ? Get(temporalTimeZoneLike, "timeZone").
        temporal_time_zone_like = TRY(temporal_time_zone_like.as_object().get(vm.names.timeZone));

        // f. If Type(temporalTimeZoneLike) is Object, then
        if (temporal_time_zone_like.is_object()) {
            // i. If temporalTimeZoneLike has an [[InitializedTemporalCalendar]] internal slot, throw a RangeError exception.
            if (is<Calendar>(temporal_time_zone_like.as_object()))
                return vm.throw_completion<RangeError>(ErrorType::TemporalUnexpectedCalendarObject);

            // ii. If ? HasProperty(temporalTimeZoneLike, "timeZone") is false, return temporalTimeZoneLike.
            if (!TRY(temporal_time_zone_like.as_object().has_property(vm.names.timeZone)))
                return &temporal_time_zone_like.as_object();
        }
    }

    // 2. Let identifier be ? ToString(temporalTimeZoneLike).
    auto identifier = TRY(temporal_time_zone_like.to_string(vm));

    // 3. Let parseResult be ? ParseTemporalTimeZoneString(identifier).
    auto parse_result = TRY(parse_temporal_time_zone_string(vm, identifier));

    // 4. If parseResult.[[Name]] is not undefined, then
    if (parse_result.name.has_value()) {
        // a. Let name be parseResult.[[Name]].
        auto name = parse_result.name.release_value();

        // b. If IsTimeZoneOffsetString(name) is false, then
        if (!is_time_zone_offset_string(name)) {
            // i. If IsAvailableTimeZoneName(name) is false, throw a RangeError exception.
            if (!is_available_time_zone_name(name))
                return vm.throw_completion<RangeError>(ErrorType::TemporalInvalidTimeZoneName, name);

            // ii. Set name to ! CanonicalizeTimeZoneName(name).
            name = MUST_OR_THROW_OOM(canonicalize_time_zone_name(vm, name));
        }

        // c. Return ! CreateTemporalTimeZone(name).
        return MUST_OR_THROW_OOM(create_temporal_time_zone(vm, name));
    }

    // 5. If parseResult.[[Z]] is true, return ! CreateTemporalTimeZone("UTC").
    if (parse_result.z)
        return MUST_OR_THROW_OOM(create_temporal_time_zone(vm, "UTC"sv));

    // 6. Return ! CreateTemporalTimeZone(parseResult.[[OffsetString]]).
    return MUST_OR_THROW_OOM(create_temporal_time_zone(vm, *parse_result.offset_string));
}

// 11.6.8 GetOffsetNanosecondsFor ( timeZone, instant ), https://tc39.es/proposal-temporal/#sec-temporal-getoffsetnanosecondsfor
ThrowCompletionOr<double> get_offset_nanoseconds_for(VM& vm, Value time_zone, Instant& instant)
{
    // 1. Let getOffsetNanosecondsFor be ? GetMethod(timeZone, "getOffsetNanosecondsFor").
    auto* get_offset_nanoseconds_for = TRY(time_zone.get_method(vm, vm.names.getOffsetNanosecondsFor));

    // 2. Let offsetNanoseconds be ? Call(getOffsetNanosecondsFor, timeZone, « instant »).
    auto offset_nanoseconds_value = TRY(call(vm, get_offset_nanoseconds_for, time_zone, &instant));

    // 3. If Type(offsetNanoseconds) is not Number, throw a TypeError exception.
    if (!offset_nanoseconds_value.is_number())
        return vm.throw_completion<TypeError>(ErrorType::IsNotA, "Offset nanoseconds value", "number");

    // 4. If IsIntegralNumber(offsetNanoseconds) is false, throw a RangeError exception.
    if (!offset_nanoseconds_value.is_integral_number())
        return vm.throw_completion<RangeError>(ErrorType::IsNotAn, "Offset nanoseconds value", "integral number");

    // 5. Set offsetNanoseconds to ℝ(offsetNanoseconds).
    auto offset_nanoseconds = offset_nanoseconds_value.as_double();

    // 6. If abs(offsetNanoseconds) ≥ nsPerDay, throw a RangeError exception.
    if (fabs(offset_nanoseconds) >= ns_per_day)
        return vm.throw_completion<RangeError>(ErrorType::TemporalInvalidOffsetNanosecondsValue);

    // 7. Return offsetNanoseconds.
    return offset_nanoseconds;
}

// 11.6.9 BuiltinTimeZoneGetOffsetStringFor ( timeZone, instant ), https://tc39.es/proposal-temporal/#sec-temporal-builtintimezonegetoffsetstringfor
ThrowCompletionOr<String> builtin_time_zone_get_offset_string_for(VM& vm, Value time_zone, Instant& instant)
{
    // 1. Let offsetNanoseconds be ? GetOffsetNanosecondsFor(timeZone, instant).
    auto offset_nanoseconds = TRY(get_offset_nanoseconds_for(vm, time_zone, instant));

    // 2. Return ! FormatTimeZoneOffsetString(offsetNanoseconds).
    return MUST_OR_THROW_OOM(format_time_zone_offset_string(vm, offset_nanoseconds));
}

// 11.6.10 BuiltinTimeZoneGetPlainDateTimeFor ( timeZone, instant, calendar ), https://tc39.es/proposal-temporal/#sec-temporal-builtintimezonegetplaindatetimefor
ThrowCompletionOr<PlainDateTime*> builtin_time_zone_get_plain_date_time_for(VM& vm, Value time_zone, Instant& instant, Object& calendar)
{
    // 1. Assert: instant has an [[InitializedTemporalInstant]] internal slot.

    // 2. Let offsetNanoseconds be ? GetOffsetNanosecondsFor(timeZone, instant).
    auto offset_nanoseconds = TRY(get_offset_nanoseconds_for(vm, time_zone, instant));

    // 3. Let result be ! GetISOPartsFromEpoch(ℝ(instant.[[Nanoseconds]])).
    auto result = get_iso_parts_from_epoch(vm, instant.nanoseconds().big_integer());

    // 4. Set result to BalanceISODateTime(result.[[Year]], result.[[Month]], result.[[Day]], result.[[Hour]], result.[[Minute]], result.[[Second]], result.[[Millisecond]], result.[[Microsecond]], result.[[Nanosecond]] + offsetNanoseconds).
    result = balance_iso_date_time(result.year, result.month, result.day, result.hour, result.minute, result.second, result.millisecond, result.microsecond, result.nanosecond + offset_nanoseconds);

    // 5. Return ? CreateTemporalDateTime(result.[[Year]], result.[[Month]], result.[[Day]], result.[[Hour]], result.[[Minute]], result.[[Second]], result.[[Millisecond]], result.[[Microsecond]], result.[[Nanosecond]], calendar).
    return create_temporal_date_time(vm, result.year, result.month, result.day, result.hour, result.minute, result.second, result.millisecond, result.microsecond, result.nanosecond, calendar);
}

// 11.6.11 BuiltinTimeZoneGetInstantFor ( timeZone, dateTime, disambiguation ), https://tc39.es/proposal-temporal/#sec-temporal-builtintimezonegetinstantfor
ThrowCompletionOr<Instant*> builtin_time_zone_get_instant_for(VM& vm, Value time_zone, PlainDateTime& date_time, StringView disambiguation)
{
    // 1. Assert: dateTime has an [[InitializedTemporalDateTime]] internal slot.

    // 2. Let possibleInstants be ? GetPossibleInstantsFor(timeZone, dateTime).
    auto possible_instants = TRY(get_possible_instants_for(vm, time_zone, date_time));

    // 3. Return ? DisambiguatePossibleInstants(possibleInstants, timeZone, dateTime, disambiguation).
    return disambiguate_possible_instants(vm, possible_instants, time_zone, date_time, disambiguation);
}

// 11.6.12 DisambiguatePossibleInstants ( possibleInstants, timeZone, dateTime, disambiguation ), https://tc39.es/proposal-temporal/#sec-temporal-disambiguatepossibleinstants
ThrowCompletionOr<Instant*> disambiguate_possible_instants(VM& vm, MarkedVector<Instant*> const& possible_instants, Value time_zone, PlainDateTime& date_time, StringView disambiguation)
{
    // 1. Assert: dateTime has an [[InitializedTemporalDateTime]] internal slot.

    // 2. Let n be possibleInstants's length.
    auto n = possible_instants.size();

    // 3. If n = 1, then
    if (n == 1) {
        // a. Return possibleInstants[0].
        return possible_instants[0];
    }

    // 4. If n ≠ 0, then
    if (n != 0) {
        // a. If disambiguation is "earlier" or "compatible", then
        if (disambiguation.is_one_of("earlier"sv, "compatible"sv)) {
            // i. Return possibleInstants[0].
            return possible_instants[0];
        }

        // b. If disambiguation is "later", then
        if (disambiguation == "later"sv) {
            // i. Return possibleInstants[n - 1].
            return possible_instants[n - 1];
        }

        // c. Assert: disambiguation is "reject".
        VERIFY(disambiguation == "reject"sv);

        // d. Throw a RangeError exception.
        return vm.throw_completion<RangeError>(ErrorType::TemporalDisambiguatePossibleInstantsRejectMoreThanOne);
    }

    // 5. Assert: n = 0.
    VERIFY(n == 0);

    // 6. If disambiguation is "reject", then
    if (disambiguation == "reject"sv) {
        // a. Throw a RangeError exception.
        return vm.throw_completion<RangeError>(ErrorType::TemporalDisambiguatePossibleInstantsRejectZero);
    }

    // 7. Let epochNanoseconds be GetUTCEpochNanoseconds(dateTime.[[ISOYear]], dateTime.[[ISOMonth]], dateTime.[[ISODay]], dateTime.[[ISOHour]], dateTime.[[ISOMinute]], dateTime.[[ISOSecond]], dateTime.[[ISOMillisecond]], dateTime.[[ISOMicrosecond]], dateTime.[[ISONanosecond]]).
    auto epoch_nanoseconds = get_utc_epoch_nanoseconds(date_time.iso_year(), date_time.iso_month(), date_time.iso_day(), date_time.iso_hour(), date_time.iso_minute(), date_time.iso_second(), date_time.iso_millisecond(), date_time.iso_microsecond(), date_time.iso_nanosecond());

    // 8. Let dayBeforeNs be epochNanoseconds - ℤ(nsPerDay).
    auto day_before_ns = BigInt::create(vm, epoch_nanoseconds.minus(ns_per_day_bigint));

    // 9. If ! IsValidEpochNanoseconds(dayBeforeNs) is false, throw a RangeError exception.
    if (!is_valid_epoch_nanoseconds(day_before_ns))
        return vm.throw_completion<RangeError>(ErrorType::TemporalInvalidEpochNanoseconds);

    // 10. Let dayBefore be ! CreateTemporalInstant(dayBeforeNs).
    auto* day_before = MUST(create_temporal_instant(vm, day_before_ns));

    // 11. Let dayAfterNs be epochNanoseconds + ℤ(nsPerDay).
    auto day_after_ns = BigInt::create(vm, epoch_nanoseconds.plus(ns_per_day_bigint));

    // 12. If ! IsValidEpochNanoseconds(dayAfterNs) is false, throw a RangeError exception.
    if (!is_valid_epoch_nanoseconds(day_after_ns))
        return vm.throw_completion<RangeError>(ErrorType::TemporalInvalidEpochNanoseconds);

    // 13. Let dayAfter be ! CreateTemporalInstant(dayAfterNs).
    auto* day_after = MUST(create_temporal_instant(vm, day_after_ns));

    // 14. Let offsetBefore be ? GetOffsetNanosecondsFor(timeZone, dayBefore).
    auto offset_before = TRY(get_offset_nanoseconds_for(vm, time_zone, *day_before));

    // 15. Let offsetAfter be ? GetOffsetNanosecondsFor(timeZone, dayAfter).
    auto offset_after = TRY(get_offset_nanoseconds_for(vm, time_zone, *day_after));

    // 16. Let nanoseconds be offsetAfter - offsetBefore.
    auto nanoseconds = offset_after - offset_before;

    // 17. If disambiguation is "earlier", then
    if (disambiguation == "earlier"sv) {
        // a. Let earlier be ? AddDateTime(dateTime.[[ISOYear]], dateTime.[[ISOMonth]], dateTime.[[ISODay]], dateTime.[[ISOHour]], dateTime.[[ISOMinute]], dateTime.[[ISOSecond]], dateTime.[[ISOMillisecond]], dateTime.[[ISOMicrosecond]], dateTime.[[ISONanosecond]], dateTime.[[Calendar]], 0, 0, 0, 0, 0, 0, 0, 0, 0, -nanoseconds, undefined).
        auto earlier = TRY(add_date_time(vm, date_time.iso_year(), date_time.iso_month(), date_time.iso_day(), date_time.iso_hour(), date_time.iso_minute(), date_time.iso_second(), date_time.iso_millisecond(), date_time.iso_microsecond(), date_time.iso_nanosecond(), date_time.calendar(), 0, 0, 0, 0, 0, 0, 0, 0, 0, -nanoseconds, nullptr));

        // b. Let earlierDateTime be ! CreateTemporalDateTime(earlier.[[Year]], earlier.[[Month]], earlier.[[Day]], earlier.[[Hour]], earlier.[[Minute]], earlier.[[Second]], earlier.[[Millisecond]], earlier.[[Microsecond]], earlier.[[Nanosecond]], dateTime.[[Calendar]]).
        auto* earlier_date_time = MUST(create_temporal_date_time(vm, earlier.year, earlier.month, earlier.day, earlier.hour, earlier.minute, earlier.second, earlier.millisecond, earlier.microsecond, earlier.nanosecond, date_time.calendar()));

        // c. Set possibleInstants to ? GetPossibleInstantsFor(timeZone, earlierDateTime).
        auto possible_instants_ = TRY(get_possible_instants_for(vm, time_zone, *earlier_date_time));

        // d. If possibleInstants is empty, throw a RangeError exception.
        if (possible_instants_.is_empty())
            return vm.throw_completion<RangeError>(ErrorType::TemporalDisambiguatePossibleInstantsEarlierZero);

        // e. Return possibleInstants[0].
        return possible_instants_[0];
    }

    // 18. Assert: disambiguation is "compatible" or "later".
    VERIFY(disambiguation.is_one_of("compatible"sv, "later"sv));

    // 19. Let later be ? AddDateTime(dateTime.[[ISOYear]], dateTime.[[ISOMonth]], dateTime.[[ISODay]], dateTime.[[ISOHour]], dateTime.[[ISOMinute]], dateTime.[[ISOSecond]], dateTime.[[ISOMillisecond]], dateTime.[[ISOMicrosecond]], dateTime.[[ISONanosecond]], dateTime.[[Calendar]], 0, 0, 0, 0, 0, 0, 0, 0, 0, nanoseconds, undefined).
    auto later = TRY(add_date_time(vm, date_time.iso_year(), date_time.iso_month(), date_time.iso_day(), date_time.iso_hour(), date_time.iso_minute(), date_time.iso_second(), date_time.iso_millisecond(), date_time.iso_microsecond(), date_time.iso_nanosecond(), date_time.calendar(), 0, 0, 0, 0, 0, 0, 0, 0, 0, nanoseconds, nullptr));

    // 20. Let laterDateTime be ! CreateTemporalDateTime(later.[[Year]], later.[[Month]], later.[[Day]], later.[[Hour]], later.[[Minute]], later.[[Second]], later.[[Millisecond]], later.[[Microsecond]], later.[[Nanosecond]], dateTime.[[Calendar]]).
    auto* later_date_time = MUST(create_temporal_date_time(vm, later.year, later.month, later.day, later.hour, later.minute, later.second, later.millisecond, later.microsecond, later.nanosecond, date_time.calendar()));

    // 21. Set possibleInstants to ? GetPossibleInstantsFor(timeZone, laterDateTime).
    auto possible_instants_ = TRY(get_possible_instants_for(vm, time_zone, *later_date_time));

    // 22. Set n to possibleInstants's length.
    n = possible_instants_.size();

    // 23. If n = 0, throw a RangeError exception.
    if (n == 0)
        return vm.throw_completion<RangeError>(ErrorType::TemporalDisambiguatePossibleInstantsZero);

    // 24. Return possibleInstants[n - 1].
    return possible_instants_[n - 1];
}

// 11.6.13 GetPossibleInstantsFor ( timeZone, dateTime ), https://tc39.es/proposal-temporal/#sec-temporal-getpossibleinstantsfor
ThrowCompletionOr<MarkedVector<Instant*>> get_possible_instants_for(VM& vm, Value time_zone, PlainDateTime& date_time)
{
    // 1. Assert: dateTime has an [[InitializedTemporalDateTime]] internal slot.

    // 2. Let possibleInstants be ? Invoke(timeZone, "getPossibleInstantsFor", « dateTime »).
    auto possible_instants = TRY(time_zone.invoke(vm, vm.names.getPossibleInstantsFor, &date_time));

    // 3. Let iteratorRecord be ? GetIterator(possibleInstants, sync).
    auto iterator = TRY(get_iterator(vm, possible_instants, IteratorHint::Sync));

    // 4. Let list be a new empty List.
    auto list = MarkedVector<Instant*> { vm.heap() };

    // 5. Let next be true.
    Object* next = nullptr;

    // 6. Repeat, while next is not false,
    do {
        // a. Set next to ? IteratorStep(iteratorRecord).
        next = TRY(iterator_step(vm, iterator));

        // b. If next is not false, then
        if (next) {
            // i. Let nextValue be ? IteratorValue(next).
            auto next_value = TRY(iterator_value(vm, *next));

            // ii. If Type(nextValue) is not Object or nextValue does not have an [[InitializedTemporalInstant]] internal slot, then
            if (!next_value.is_object() || !is<Instant>(next_value.as_object())) {
                // 1. Let completion be ThrowCompletion(a newly created TypeError object).
                auto completion = vm.throw_completion<TypeError>(ErrorType::NotAnObjectOfType, "Temporal.Instant");

                // 2. Return ? IteratorClose(iteratorRecord, completion).
                return iterator_close(vm, iterator, move(completion));
            }

            // iii. Append nextValue to the end of the List list.
            list.append(static_cast<Instant*>(&next_value.as_object()));
        }
    } while (next != nullptr);

    // 7. Return list.
    return { move(list) };
}

// 11.6.14 TimeZoneEquals ( one, two ), https://tc39.es/proposal-temporal/#sec-temporal-timezoneequals
ThrowCompletionOr<bool> time_zone_equals(VM& vm, Object& one, Object& two)
{
    // 1. If one and two are the same Object value, return true.
    if (&one == &two)
        return true;

    // 2. Let timeZoneOne be ? ToString(one).
    auto time_zone_one = TRY(Value(&one).to_string(vm));

    // 3. Let timeZoneTwo be ? ToString(two).
    auto time_zone_two = TRY(Value(&two).to_string(vm));

    // 4. If timeZoneOne is timeZoneTwo, return true.
    if (time_zone_one == time_zone_two)
        return true;

    // 5. Return false.
    return false;
}

}
