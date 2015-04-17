
assertInterpolation({
  property: 'flex',
  from: '1 1 0%',
  to: '2 2 100%'
}, [
  {at: -5, is: '0 0 0%'},
  {at: -0.3, is: '0.7 0.7 0%'},
  {at: 0, is: '1 1 0%'},
  {at: 0.3, is: '1.3 1.3 30%'},
  {at: 0.6, is: '1.6 1.6 60%'},
  {at: 1, is: '2 2 100%'},
  {at: 1.5, is: '2.5 2.5 150%'}
]);
assertInterpolation({
  property: 'flex',
  from: '0 0 100%',
  to: '1 1 100%'
}, [
  {at: -0.3, is: '0 0 100%'},
  {at: 0, is: '0 0 100%'},
  {at: 0.4, is: '0 0 100%'},
  {at: 0.6, is: '1 1 100%'},
  {at: 1, is: '1 1 100%'},
  {at: 1.5, is: '1 1 100%'}
]);
