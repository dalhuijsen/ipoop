
async_test(function(t) {
    assert_equals(document.timeline.getAnimationPlayers().length, 0);
    assert_equals(container.getAnimationPlayers().length, 0);
    assert_equals(element.getAnimationPlayers().length, 0);

    element.className = 'cssAnimation';
    onload = function () {
        t.step(function() {
            var players = document.timeline.getAnimationPlayers();
            assert_equals(players.length, 1);
            assert_equals(container.getAnimationPlayers().length, 0);
            assert_equals(element.getAnimationPlayers().length, 1);

            players[0].finish();
            assert_equals(document.timeline.getAnimationPlayers().length, 0);
            assert_equals(container.getAnimationPlayers().length, 0);
            assert_equals(element.getAnimationPlayers().length, 0);
            t.done();
        });
    }
}, 'getAnimationPlayers() with cssanimations');

